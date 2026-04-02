#include "AudioEngine.h"
#include "Modules/ADSRModule.h"
#include "Modules/FilterModule.h"
#include "Modules/LFOModule.h"
#include "Modules/OscillatorModule.h"
#include "Modules/VCAModule.h"
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
