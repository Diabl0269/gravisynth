#include "../Source/AudioEngine.h"
#include "../Source/Modules/ADSRModule.h"
#include "../Source/Modules/FilterModule.h"
#include "../Source/Modules/LFOModule.h"
#include "../Source/Modules/OscillatorModule.h"
#include <gtest/gtest.h>

class ModMatrixTest : public ::testing::Test {
protected:
    void SetUp() override { engine.initialise(); }

    AudioEngine engine;
};

TEST_F(ModMatrixTest, AddModRoutingWorks) {
    auto& graph = engine.getGraph();

    // Clear graph to have a clean state
    graph.clear();

    auto lfoNode = graph.addNode(std::make_unique<LFOModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    ASSERT_NE(lfoNode, nullptr);
    ASSERT_NE(filterNode, nullptr);

    engine.addModRouting(lfoNode->nodeID, 0, filterNode->nodeID, 1); // 1 = Cutoff

    auto routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 1);
    EXPECT_EQ(routings[0].sourceNodeID, lfoNode->nodeID);
    EXPECT_EQ(routings[0].destNodeID, filterNode->nodeID);
    EXPECT_EQ(routings[0].destChannelIndex, 1);
}

TEST_F(ModMatrixTest, RemoveModRoutingWorks) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto lfoNode = graph.addNode(std::make_unique<LFOModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    engine.addModRouting(lfoNode->nodeID, 0, filterNode->nodeID, 1);
    auto routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 1);

    engine.removeModRouting(routings[0].attenuverterNodeID);
    EXPECT_EQ(engine.getActiveModRoutings().size(), 0);
}

TEST_F(ModMatrixTest, MetadataPopulation) {
    OscillatorModule osc;
    auto targets = osc.getModulationTargets();

    bool foundPitch = false;
    for (const auto& t : targets) {
        if (t.name == "Pitch")
            foundPitch = true;
    }
    EXPECT_TRUE(foundPitch);
}
TEST_F(ModMatrixTest, SidechainModulationWorks) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto lfoNode = graph.addNode(std::make_unique<LFOModule>());
    auto envNode = graph.addNode(std::make_unique<ADSRModule>("Env"));
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    // 1. Primary Routing: LFO -> Attenuverter A -> Filter Cutoff
    engine.addModRouting(lfoNode->nodeID, 0, filterNode->nodeID, 1);
    auto routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 1);
    auto attenuverterA = routings[0].attenuverterNodeID;

    // 2. Sidechain Routing: Env -> Attenuverter B -> Attenuverter A (Amount)
    engine.addModRouting(envNode->nodeID, 0, attenuverterA, 1); // 1 = Amount target
    routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 2);

    // Verify connections
    bool foundSidechain = false;
    for (const auto& r : routings) {
        if (r.destNodeID == attenuverterA && r.destChannelIndex == 1) {
            foundSidechain = true;
            EXPECT_EQ(r.sourceNodeID, envNode->nodeID);
        }
    }
    EXPECT_TRUE(foundSidechain);

    // 3. Process verification (Mock)
    // We could push samples through the graph here, but a connectivity test
    // already proves the Matrix is doing its job. The rest is internal to AttenuverterModule.
}

TEST_F(ModMatrixTest, BypassModRoutingWorks) {
    auto& graph = engine.getGraph();
    graph.clear();

    auto lfoNode = graph.addNode(std::make_unique<LFOModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    engine.addModRouting(lfoNode->nodeID, 0, filterNode->nodeID, 1);
    auto routings = engine.getActiveModRoutings();
    ASSERT_EQ(routings.size(), 1);
    auto attNodeID = routings[0].attenuverterNodeID;

    EXPECT_FALSE(engine.isModBypassed(attNodeID));

    engine.toggleModBypass(attNodeID);
    EXPECT_TRUE(engine.isModBypassed(attNodeID));

    routings = engine.getActiveModRoutings();
    EXPECT_TRUE(routings[0].isBypassed);

    engine.toggleModBypass(attNodeID);
    EXPECT_FALSE(engine.isModBypassed(attNodeID));
}
