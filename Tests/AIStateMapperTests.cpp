#include "../Source/AI/AIStateMapper.h"
#include "../Source/Modules/FilterModule.h"
#include "../Source/Modules/OscillatorModule.h"
#include "../Source/Modules/VCAModule.h"
#include <gtest/gtest.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Helper function to create a basic graph for testing
static void createBasicGraph(juce::AudioProcessorGraph& graph) {
    graph.clear();

    auto audioInputNode = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
    auto audioOutputNode = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));
    auto oscNode = graph.addNode(std::make_unique<OscillatorModule>());
    auto filterNode = graph.addNode(std::make_unique<FilterModule>());

    // Connect input to osc, osc to filter, filter to output
    graph.addConnection({{audioInputNode->nodeID, 0}, {oscNode->nodeID, 0}});
    graph.addConnection({{oscNode->nodeID, 0}, {filterNode->nodeID, 0}});
    graph.addConnection({{filterNode->nodeID, 0}, {audioOutputNode->nodeID, 0}});
}

TEST(AIStateMapperTest, GraphToJSONRoundTrip) {
    juce::AudioProcessorGraph originalGraph;
    createBasicGraph(originalGraph);

    // Save to JSON
    juce::var json = gsynth::AIStateMapper::graphToJSON(originalGraph);

    // Create a new graph and load from JSON
    juce::AudioProcessorGraph newGraph;
    bool success = gsynth::AIStateMapper::applyJSONToGraph(json, newGraph, true);

    ASSERT_TRUE(success);
    ASSERT_EQ(originalGraph.getNumNodes(), newGraph.getNumNodes());
    ASSERT_EQ(originalGraph.getConnections().size(), newGraph.getConnections().size());

    // Basic check: ensure node types match
    for (int i = 0; i < originalGraph.getNumNodes(); ++i) {
        auto originalNode = originalGraph.getNodes().getUnchecked(i);
        auto newNode = newGraph.getNodes().getUnchecked(i);
        ASSERT_EQ(originalNode->getProcessor()->getName(), newNode->getProcessor()->getName());
    }
}

TEST(AIStateMapperTest, ApplyJSONToGraphClearsExisting) {
    juce::AudioProcessorGraph graph;

    // JSON with a simple Filter and Audio Output
    juce::var json = juce::JSON::parse(R"({"nodes":[
        {"id":100,"type":"Filter","params":{"cutoff":0.5,"resonance":0.1}},
        {"id":101,"type":"Audio Output","params":{}}
    ],"connections":[]})");

    // Test 1: Apply patch, clearing existing (graph should be empty initially)
    bool success_clear = gsynth::AIStateMapper::applyJSONToGraph(json, graph, true);
    ASSERT_TRUE(success_clear);
    ASSERT_EQ(graph.getNumNodes(), 2); // Only nodes from JSON should exist

    // Clear the graph before the second test to ensure a clean state
    graph.clear();

    // Test 2: Apply patch, without clearing existing (should add to existing graph)
    graph.addNode(std::make_unique<OscillatorModule>()); // Add an initial node
    bool success_no_clear = gsynth::AIStateMapper::applyJSONToGraph(json, graph, false);
    ASSERT_TRUE(success_no_clear);
    ASSERT_EQ(graph.getNumNodes(), 3); // 1 (Oscillator) + 2 (from JSON) = 3
}

TEST(AIStateMapperTest, InvalidJSONReturnsFalse) {
    juce::AudioProcessorGraph graph;

    // Not an object
    juce::var invalidJson1 = juce::JSON::parse(R"("not an object")");
    ASSERT_FALSE(gsynth::AIStateMapper::applyJSONToGraph(invalidJson1, graph));

    // Missing "nodes"
    juce::var invalidJson2 = juce::JSON::parse(R"({"connections":[]})");
    ASSERT_FALSE(gsynth::AIStateMapper::applyJSONToGraph(invalidJson2, graph));

    // "nodes" is not an array
    juce::var invalidJson3 = juce::JSON::parse(R"({"nodes":"not an array"})");
    ASSERT_FALSE(gsynth::AIStateMapper::applyJSONToGraph(invalidJson3, graph));
}

TEST(AIStateMapperTest, ParameterValidationClamping) {
    juce::AudioProcessorGraph graph;
    juce::var json = juce::JSON::parse(
        R"({"nodes":[{"id":1,"type":"Oscillator","params":{"waveform":0,"frequency":2.0}}],"connections":[]})"); // Freq
                                                                                                                 // > 1.0

    gsynth::AIStateMapper::applyJSONToGraph(json, graph, true);

    auto oscNode = graph.getNodes().getUnchecked(0);
    ASSERT_NE(oscNode, nullptr);
    auto oscProcessor = dynamic_cast<OscillatorModule*>(oscNode->getProcessor());
    ASSERT_NE(oscProcessor, nullptr);

    float frequencyParamValue = -1.0f;
    for (auto* param : oscProcessor->getParameters()) {
        if (auto* floatParam = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
            if (floatParam->paramID == "frequency") {
                frequencyParamValue = floatParam->getValue();
                break;
            }
        }
    }
    ASSERT_NE(frequencyParamValue, -1.0f); // Ensure frequency parameter was found

    // Parameter value should be clamped between 0.0 and 1.0
    ASSERT_NEAR(frequencyParamValue, 1.0f, 0.001f); // Should be clamped to 1.0
}

TEST(AIStateMapperTest, UnknownModuleTypeLogsErrorAndSkips) {
    juce::AudioProcessorGraph graph;
    juce::var json = juce::JSON::parse(R"({"nodes":[{"id":1,"type":"UnknownModule"}],"connections":[]})");

    // Capture logs to ensure error is logged
    class LogCatcher : public juce::Logger {
    public:
        LogCatcher() { juce::Logger::setCurrentLogger(this); }
        ~LogCatcher() override { juce::Logger::setCurrentLogger(nullptr); }
        void logMessage(const juce::String& message) override { lastMessage = message; }
        juce::String lastMessage;
    };
    LogCatcher logger;

    bool success_unknown_module = gsynth::AIStateMapper::applyJSONToGraph(json, graph, true);
    ASSERT_TRUE(success_unknown_module); // Should still return true if valid JSON, just skips the node
    ASSERT_EQ(graph.getNumNodes(), 0);   // Unknown module should not be added
    ASSERT_TRUE(logger.lastMessage.contains("Unknown module type"));
}
