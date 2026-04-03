#include "AI/AIStateMapper.h"
#include "AudioEngine.h"
#include "Modules/ADSRModule.h"
#include "Modules/FilterModule.h"
#include "Modules/LFOModule.h"
#include "Modules/OscillatorModule.h"
#include "Modules/VCAModule.h"
#include "PresetManager.h"
#include <gtest/gtest.h>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { engine.initialise(); }

    AudioEngine engine;
};

TEST_F(IntegrationTest, OscToFilterToVCA) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto oscNode = graph.addNode(std::make_unique<OscillatorModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());
    auto vcaNode = graph.addNode(std::make_unique<VCAModule>());

    ASSERT_NE(oscNode, nullptr);
    ASSERT_NE(filterNode, nullptr);
    ASSERT_NE(vcaNode, nullptr);

    // Osc out 0 -> Filter in 0
    EXPECT_TRUE(graph.addConnection({{oscNode->nodeID, 0}, {filterNode->nodeID, 0}}));
    // Filter out 0 -> VCA in 0
    EXPECT_TRUE(graph.addConnection({{filterNode->nodeID, 0}, {vcaNode->nodeID, 0}}));
}

TEST_F(IntegrationTest, LFOModulatesFilterCutoff) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto lfoNode = graph.addNode(std::make_unique<LFOModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    ASSERT_NE(lfoNode, nullptr);
    ASSERT_NE(filterNode, nullptr);

    engine.addModRouting(lfoNode->nodeID, 0, filterNode->nodeID, 1);

    auto routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 1);
    EXPECT_EQ(routings[0].sourceNodeID, lfoNode->nodeID);
    EXPECT_EQ(routings[0].destNodeID, filterNode->nodeID);
    EXPECT_EQ(routings[0].destChannelIndex, 1);
}

TEST_F(IntegrationTest, ADSREnvelopeToVCA) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto adsrNode = graph.addNode(std::make_unique<ADSRModule>());
    auto vcaNode = graph.addNode(std::make_unique<VCAModule>());

    ASSERT_NE(adsrNode, nullptr);
    ASSERT_NE(vcaNode, nullptr);

    // ADSR out 0 -> VCA CV in (channel 1)
    EXPECT_TRUE(graph.addConnection({{adsrNode->nodeID, 0}, {vcaNode->nodeID, 1}}));
}

TEST_F(IntegrationTest, MultipleModulationsOnSameTarget) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto lfo1Node = graph.addNode(std::make_unique<LFOModule>());
    auto lfo2Node = graph.addNode(std::make_unique<LFOModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    ASSERT_NE(lfo1Node, nullptr);
    ASSERT_NE(lfo2Node, nullptr);
    ASSERT_NE(filterNode, nullptr);

    engine.addModRouting(lfo1Node->nodeID, 0, filterNode->nodeID, 1);
    engine.addModRouting(lfo2Node->nodeID, 0, filterNode->nodeID, 1);

    auto routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 2);
}

TEST_F(IntegrationTest, PresetLoadRestoresModRoutings) {
    auto& graph = engine.getGraph();
    ASSERT_TRUE(gsynth::PresetManager::loadDefaultPreset(graph));

    // The default preset has Attenuverter nodes for mod routing
    bool hasAttenuverter = false;
    for (auto* node : graph.getNodes()) {
        if (node->getProcessor()->getName().contains("Attenuverter") ||
            node->getProcessor()->getName().contains("Mod Slot")) {
            hasAttenuverter = true;
            break;
        }
    }
    EXPECT_TRUE(hasAttenuverter) << "Default preset should contain Attenuverter nodes for modulation routing";

    // Verify connections exist
    EXPECT_GT(graph.getConnections().size(), 0);
}

TEST_F(IntegrationTest, SaveLoadRoundTripPreservesState) {
    auto& graph = engine.getGraph();

    // Load a preset
    ASSERT_TRUE(gsynth::PresetManager::loadPreset(1, graph)); // Simple Lead

    int originalNodeCount = graph.getNumNodes();
    int originalConnectionCount = (int)graph.getConnections().size();
    ASSERT_GT(originalNodeCount, 0);
    ASSERT_GT(originalConnectionCount, 0);

    // Serialize to JSON
    auto json = gsynth::AIStateMapper::graphToJSON(graph);

    // Load into a fresh graph
    juce::AudioProcessorGraph newGraph;
    ASSERT_TRUE(gsynth::AIStateMapper::applyJSONToGraph(json, newGraph, true));

    // Verify node and connection counts match
    EXPECT_EQ(newGraph.getNumNodes(), originalNodeCount);
    EXPECT_EQ((int)newGraph.getConnections().size(), originalConnectionCount);

    // Verify node types match
    for (int i = 0; i < originalNodeCount; ++i) {
        auto origNode = graph.getNodes().getUnchecked(i);
        auto newNode = newGraph.getNodes().getUnchecked(i);
        EXPECT_EQ(origNode->getProcessor()->getName(), newNode->getProcessor()->getName())
            << "Node type mismatch at index " << i;
    }
}

TEST_F(IntegrationTest, AllPresetsLoadAndHaveSignalPath) {
    auto& graph = engine.getGraph();
    auto presetNames = gsynth::PresetManager::getPresetNames();

    for (int i = 0; i < presetNames.size(); ++i) {
        graph.clear();
        ASSERT_TRUE(gsynth::PresetManager::loadPreset(i, graph))
            << "Failed to load preset: " << presetNames[i].toStdString();

        // Every preset should have an audio output
        bool hasAudioOutput = false;
        for (auto* node : graph.getNodes()) {
            if (node->getProcessor()->getName() == "Audio Output")
                hasAudioOutput = true;
        }
        EXPECT_TRUE(hasAudioOutput) << "Preset '" << presetNames[i].toStdString() << "' missing Audio Output";

        // Every preset should have connections
        EXPECT_GT(graph.getConnections().size(), 0)
            << "Preset '" << presetNames[i].toStdString() << "' has no connections";
    }
}
