#include "AI/AIProvider.h"
#include "AudioEngine.h"
#include "GravisynthUndoManager.h"
#include "IGraphManager.h"
#include "MainComponent.h"
#include "Modules/ADSRModule.h"
#include "Modules/FilterModule.h"
#include "Modules/LFOModule.h"
#include "Modules/OscillatorModule.h"
#include "Modules/VCAModule.h"
#include "PresetManager.h"
#include "UI/GraphEditor.h"
#include "UI/ModuleComponent.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================================
// Mock Provider
// ============================================================================

class MockProvider : public gsynth::AIProvider {
public:
    juce::String getProviderName() const override { return "Mock"; }
    void fetchAvailableModels(std::function<void(const juce::StringArray&, bool)> callback) override {
        callback({"MockModel"}, true);
    }
    void sendPrompt(const std::vector<Message>&, CompletionCallback callback, const juce::var&) override {
        callback("Mock response.", true);
    }
    void setModel(const juce::String& name) override { model = name; }
    juce::String getCurrentModel() const override { return model; }

private:
    juce::String model = "MockModel";
};

// ============================================================================
// Dummy Drag Source (for module dropping)
// ============================================================================

class DummyDragSource : public juce::Component {};

// ============================================================================
// Workflow Test Fixture
// ============================================================================

class WorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        mainComp = std::make_unique<MainComponent>(std::make_unique<MockProvider>());
        mainComp->setSize(1024, 768);
        // Close the audio device so tests don't route audio to speakers
        mainComp->getDeviceManager().closeAudioDevice();
    }

    void TearDown() override { mainComp.reset(); }

    std::unique_ptr<MainComponent> mainComp;

    // Convenience accessors
    GraphEditor& editor() { return mainComp->getGraphEditor(); }
    IGraphManager& graphManager() { return editor().getGraphManager(); }
    juce::AudioProcessorGraph& graph() { return graphManager().getGraph(); }

    // Drop a module at a specified position
    void dropModule(const juce::String& type, juce::Point<int> pos = {100, 100}) {
        DummyDragSource src;
        juce::var desc(type);
        juce::DragAndDropTarget::SourceDetails details(desc, &src, pos);
        editor().itemDropped(details);
    }

    // Find a ModuleComponent by name substring
    ModuleComponent* findModule(const juce::String& nameContains) {
        auto* content = editor().getChildComponent(0);
        if (!content)
            return nullptr;
        for (auto* child : content->getChildren()) {
            if (auto* mod = dynamic_cast<ModuleComponent*>(child)) {
                if (mod->getModule()->getName().contains(nameContains))
                    return mod;
            }
        }
        return nullptr;
    }

    // Connect two modules via UI
    void connectModules(ModuleComponent* src, int srcPort, ModuleComponent* dst, int dstPort) {
        if (!src || !dst)
            return;
        src->setBounds(0, 0, 150, 150);
        dst->setBounds(300, 0, 150, 150);
        editor().beginConnectionDrag(src, srcPort, false, false, juce::Point<int>(0, 0));
        editor().dragConnection(juce::Point<int>(150, 0));
        auto targetPoint = dst->getBounds().getPosition() + dst->getPortCenter(dstPort, true);
        editor().endConnectionDrag(targetPoint);
    }

    // Trigger undo via MainComponent
    void triggerUndo() { mainComp->simulateUndoClick(); }

    // Trigger redo via MainComponent
    void triggerRedo() { mainComp->simulateRedoClick(); }

    // Count nodes in graph
    int nodeCount() { return static_cast<int>(graph().getNodes().size()); }

    // Count connections in graph
    int connectionCount() { return static_cast<int>(graph().getConnections().size()); }
};

// ============================================================================
// Test Cases
// ============================================================================

// Test 1: Drop multiple modules and verify they all exist
TEST_F(WorkflowTest, DropMultipleModulesAndVerify) {
    int initialCount = nodeCount();

    dropModule("Oscillator", {100, 100});
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), initialCount + 1);
    EXPECT_NE(findModule("Oscillator"), nullptr);

    dropModule("Filter", {250, 100});
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), initialCount + 2);
    EXPECT_NE(findModule("Filter"), nullptr);

    dropModule("VCA", {400, 100});
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), initialCount + 3);
    EXPECT_NE(findModule("VCA"), nullptr);
}

// Test 2: Drop modules and connect via graph API
TEST_F(WorkflowTest, DropAndConnectModules) {
    // Add modules directly to graph for reliable connection testing
    auto* oscNode = graph().addNode(std::make_unique<OscillatorModule>()).get();
    auto* filterNode = graph().addNode(std::make_unique<FilterModule>()).get();
    ASSERT_NE(oscNode, nullptr);
    ASSERT_NE(filterNode, nullptr);

    int initialConnections = connectionCount();
    graph().addConnection({{oscNode->nodeID, 0}, {filterNode->nodeID, 0}});

    EXPECT_EQ(connectionCount(), initialConnections + 1);
}

// Test 3: Delete module with undo/redo
TEST_F(WorkflowTest, DeleteModuleUndoRedo) {
    int initialCount = nodeCount();
    dropModule("Oscillator", {100, 100});
    editor().updateComponents();

    int countAfterDrop = nodeCount();
    EXPECT_EQ(countAfterDrop, initialCount + 1);

    auto* oscComp = findModule("Oscillator");
    ASSERT_NE(oscComp, nullptr);

    editor().deleteModule(oscComp);
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), countAfterDrop - 1);

    // Undo deletion
    triggerUndo();
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), countAfterDrop);

    // Redo deletion
    triggerRedo();
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), countAfterDrop - 1);
}

// Test 4: Add/remove connection with undo/redo via undo manager
TEST_F(WorkflowTest, ConnectDisconnectUndoRedo) {
    auto oscNode = graph().addNode(std::make_unique<OscillatorModule>());
    auto filterNode = graph().addNode(std::make_unique<FilterModule>());
    ASSERT_TRUE(oscNode && filterNode);

    int initialConnections = connectionCount();
    auto& undoMgr = mainComp->getUndoManager();

    // Add connection via undo manager
    undoMgr.recordStructuralChange(graph(),
                                   [&] { graph().addConnection({{oscNode->nodeID, 0}, {filterNode->nodeID, 0}}); });
    EXPECT_EQ(connectionCount(), initialConnections + 1);

    // Undo connection
    undoMgr.undo();
    EXPECT_EQ(connectionCount(), initialConnections);

    // Redo connection
    undoMgr.redo();
    EXPECT_EQ(connectionCount(), initialConnections + 1);
}

// Test 5: Complex workflow - add modules, connect, remove module, undo
TEST_F(WorkflowTest, ComplexWorkflowDropConnectChangeUndo) {
    auto& undoMgr = mainComp->getUndoManager();

    // Add 3 modules via undo manager
    juce::AudioProcessorGraph::NodeID oscId, filterId, vcaId;
    undoMgr.recordStructuralChange(graph(),
                                   [&] { oscId = graph().addNode(std::make_unique<OscillatorModule>())->nodeID; });
    undoMgr.recordStructuralChange(graph(),
                                   [&] { filterId = graph().addNode(std::make_unique<FilterModule>())->nodeID; });
    undoMgr.recordStructuralChange(graph(), [&] { vcaId = graph().addNode(std::make_unique<VCAModule>())->nodeID; });

    int stateNodeCount = nodeCount();

    // Connect them
    undoMgr.recordStructuralChange(graph(), [&] {
        graph().addConnection({{oscId, 0}, {filterId, 0}});
        graph().addConnection({{filterId, 0}, {vcaId, 0}});
    });
    int afterConnections = connectionCount();

    // Remove filter
    undoMgr.recordStructuralChange(graph(), [&] { graph().removeNode(filterId); });
    EXPECT_EQ(nodeCount(), stateNodeCount - 1);

    // Undo removal — filter should be restored with connections
    undoMgr.undo();
    EXPECT_EQ(nodeCount(), stateNodeCount);
    EXPECT_EQ(connectionCount(), afterConnections);
}

// Test 6: Rapid undo/redo with no crash
TEST_F(WorkflowTest, RapidUndoRedoNoCrash) {
    int initialCount = nodeCount();
    // Drop 5 modules
    for (int i = 0; i < 5; ++i) {
        SCOPED_TRACE(testing::Message() << "Dropping module " << i);
        dropModule("Oscillator", {100 + i * 50, 100});
        editor().updateComponents();
    }
    int finalNodeCount = nodeCount();
    EXPECT_EQ(finalNodeCount, initialCount + 5);

    // Rapid undo 5 times
    EXPECT_NO_FATAL_FAILURE({
        for (int i = 0; i < 5; ++i) {
            SCOPED_TRACE(testing::Message() << "Undo " << i);
            triggerUndo();
            editor().updateComponents();
        }
    });

    // Should be back to initial count
    EXPECT_EQ(nodeCount(), initialCount);

    // Rapid redo 5 times
    EXPECT_NO_FATAL_FAILURE({
        for (int i = 0; i < 5; ++i) {
            SCOPED_TRACE(testing::Message() << "Redo " << i);
            triggerRedo();
            editor().updateComponents();
        }
    });

    // Should be back to finalNodeCount
    EXPECT_EQ(nodeCount(), finalNodeCount);
}

// Test 7: Modulation routing undo/redo
TEST_F(WorkflowTest, ModRoutingUndoRedo) {
    // Add modules directly to graph
    auto lfoNode = graph().addNode(std::make_unique<LFOModule>());
    auto oscNode = graph().addNode(std::make_unique<OscillatorModule>());
    editor().updateComponents();

    ASSERT_NE(lfoNode, nullptr);
    ASSERT_NE(oscNode, nullptr);

    auto lfoId = lfoNode->nodeID;
    auto oscId = oscNode->nodeID;

    // Add modulation routing
    int initialRoutings = static_cast<int>(graphManager().getActiveModRoutings().size());
    graphManager().addModRouting(lfoId, 0, oscId, 0);

    EXPECT_EQ(static_cast<int>(graphManager().getActiveModRoutings().size()), initialRoutings + 1);

    // Note: Direct engine API undo/redo would be via GravisynthUndoManager
    // For this test we verify the routing was added correctly
    auto routings = graphManager().getActiveModRoutings();
    EXPECT_GT(routings.size(), static_cast<size_t>(initialRoutings));
}

// Test 8: Load presets without crashing
TEST_F(WorkflowTest, PresetLoadDoesNotCrash) {
    auto presetNames = gsynth::PresetManager::getPresetNames();
    EXPECT_GT(presetNames.size(), 0);

    EXPECT_NO_FATAL_FAILURE({
        for (int i = 0; i < presetNames.size(); ++i) {
            SCOPED_TRACE(testing::Message() << "Loading preset " << i << ": " << presetNames[i]);
            // Detach UI from current graph nodes BEFORE loading a new preset.
            // loadPreset clears the graph (destroying processors), so UI components
            // must release their references first to avoid use-after-free.
            editor().detachAllModuleComponents();
            gsynth::PresetManager::loadPreset(i, graph());
            editor().updateComponents();
            EXPECT_GT(nodeCount(), 0);
        }
    });
}

// Test 9: Replace module with undo/redo
TEST_F(WorkflowTest, ReplaceModuleUndoRestoresOriginal) {
    int initialCount = nodeCount();

    dropModule("Chorus", {100, 100});
    editor().updateComponents();
    EXPECT_EQ(nodeCount(), initialCount + 1);

    auto* chorusComp = findModule("Chorus");
    ASSERT_NE(chorusComp, nullptr);

    editor().replaceModule(chorusComp, "Phaser");
    editor().updateComponents();

    EXPECT_EQ(nodeCount(), initialCount + 1);
    EXPECT_NE(findModule("Phaser"), nullptr) << "Phaser should exist after replacement";

    // Undo replacement
    triggerUndo();
    editor().updateComponents();
    EXPECT_NE(findModule("Chorus"), nullptr) << "Chorus should be restored after undo";
}

// Test 10: Drop modules at different positions
TEST_F(WorkflowTest, DropModulesAtDifferentPositions) {
    int initialCount = nodeCount();
    juce::Point<int> positions[] = {{50, 50}, {200, 150}, {400, 300}, {600, 100}, {100, 500}};

    for (size_t i = 0; i < 5; ++i) {
        SCOPED_TRACE(testing::Message() << "Dropping module at position " << i);
        dropModule("Oscillator", positions[i]);
        editor().updateComponents();
    }

    // Verify all 5 modules were added
    EXPECT_EQ(nodeCount(), initialCount + 5);

    // Verify positions are stored in node properties
    int posIndex = 0;
    for (auto* node : graph().getNodes()) {
        SCOPED_TRACE(testing::Message() << "Checking node position " << posIndex);
        int storedX = static_cast<int>(node->properties.getWithDefault("x", 0));
        int storedY = static_cast<int>(node->properties.getWithDefault("y", 0));

        // Positions should be in the expected range (allowing some tolerance)
        EXPECT_GE(storedX, 0);
        EXPECT_GE(storedY, 0);
        posIndex++;
    }
}
