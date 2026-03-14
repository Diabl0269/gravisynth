#include "../Source/Modules/FilterModule.h"
#include "../Source/Modules/OscillatorModule.h"
#include "../Source/UI/GraphEditor.h"
#include "../Source/UI/ModuleComponent.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Define a dummy component to act as the drag source
class DummyDragSource : public juce::Component {};

class GraphEditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        juce::MessageManager::getInstance();
        engine = std::make_unique<AudioEngine>();
        editor = std::make_unique<GraphEditor>(*engine);
        editor->setSize(800, 600);
    }

    void TearDown() override {
        editor.reset();
        engine.reset();
        juce::MessageManager::deleteInstance();
        juce::DeletedAtShutdown::deleteAll();
    }

    std::unique_ptr<AudioEngine> engine;
    std::unique_ptr<GraphEditor> editor;
    DummyDragSource dummySource;
};

TEST_F(GraphEditorTest, InitializationAndResizing) {
    EXPECT_TRUE(editor->isModMatrixVisible());
    EXPECT_NO_THROW(editor->resized());
}

TEST_F(GraphEditorTest, ToggleModMatrixVisibility) {
    EXPECT_TRUE(editor->isModMatrixVisible());
    editor->toggleModMatrixVisibility();
    EXPECT_FALSE(editor->isModMatrixVisible());
    editor->toggleModMatrixVisibility();
    EXPECT_TRUE(editor->isModMatrixVisible());
}

TEST_F(GraphEditorTest, DropModuleCreatesNode) {
    juce::var description("Oscillator");
    juce::DragAndDropTarget::SourceDetails details(description, &dummySource, juce::Point<int>(100, 100));

    EXPECT_TRUE(editor->isInterestedInDragSource(details));

    // Initial nodes (Audio In/Out, Midi In)
    auto initialNodeCount = engine->getGraph().getNodes().size();

    editor->itemDropped(details);

    // Should have added an Oscillator node
    EXPECT_EQ(engine->getGraph().getNodes().size(), initialNodeCount + 1);

    // Find the newly added node
    bool foundOsc = false;
    for (auto* node : engine->getGraph().getNodes()) {
        if (node->getProcessor()->getName() == "Oscillator") {
            foundOsc = true;
            // Verify position properties were set
            EXPECT_EQ(static_cast<int>(node->properties.getWithDefault("x", 0)), 100);
            EXPECT_EQ(static_cast<int>(node->properties.getWithDefault("y", 0)), 100);
            break;
        }
    }
    EXPECT_TRUE(foundOsc);
}

TEST_F(GraphEditorTest, DragConnectionCreatesLink) {
    // Add two modules manually to the graph
    auto oscNode = engine->getGraph().addNode(std::make_unique<OscillatorModule>());
    auto filterNode = engine->getGraph().addNode(std::make_unique<FilterModule>());

    editor->updateComponents();

    // Find the ModuleComponents representing these nodes
    ModuleComponent* oscComp = nullptr;
    ModuleComponent* filterComp = nullptr;

    // We have to reach into the content component or find them by children
    for (auto* child : editor->getChildren()) {
        if (auto* content = dynamic_cast<juce::Component*>(child)) {
            for (auto* contentChild : content->getChildren()) {
                if (auto* mod = dynamic_cast<ModuleComponent*>(contentChild)) {
                    if (mod->getModule() == oscNode->getProcessor())
                        oscComp = mod;
                    if (mod->getModule() == filterNode->getProcessor())
                        filterComp = mod;
                }
            }
        }
    }

    // A hack because GraphContentComponent is private, we just assume we found them
    // If not, the test will correctly fail
    if (!oscComp || !filterComp) {
        // Find them another way since GraphContentComponent is private.
        // It's the first child of GraphEditor.
        auto* content = editor->getChildComponent(0);
        for (auto* contentChild : content->getChildren()) {
            if (auto* mod = dynamic_cast<ModuleComponent*>(contentChild)) {
                if (mod->getModule() == oscNode->getProcessor())
                    oscComp = mod;
                if (mod->getModule() == filterNode->getProcessor())
                    filterComp = mod;
            }
        }
    }

    ASSERT_NE(oscComp, nullptr);
    ASSERT_NE(filterComp, nullptr);

    // Setup positions for predictable hit testing
    oscComp->setBounds(0, 0, 100, 100);
    filterComp->setBounds(200, 0, 100, 100);

    // Simulate drag from osc out (channel 0) to filter in
    editor->beginConnectionDrag(oscComp, 0, false, false, juce::Point<int>(0, 0));
    editor->dragConnection(juce::Point<int>(50, 0));

    // Simulate drop on the filter component's input port area
    // Assuming port 0 (audio input) is somewhere near the top left of the filter component
    auto filterTargetPoint = filterComp->getBounds().getPosition() + filterComp->getPortCenter(0, true);

    // This requires coordinate mapping. GraphEditor expects screen pos.
    // For unit tests, local pos = screen pos roughly if not embedded.
    editor->endConnectionDrag(filterTargetPoint);

    // The graph should now have a connection between them
    bool connectionFound = false;
    for (auto& conn : engine->getGraph().getConnections()) {
        if (conn.source.nodeID == oscNode->nodeID && conn.destination.nodeID == filterNode->nodeID) {
            connectionFound = true;
            break;
        }
    }

    EXPECT_TRUE(connectionFound);
}
