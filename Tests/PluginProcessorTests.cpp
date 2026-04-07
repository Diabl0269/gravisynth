#include "../Source/Plugin/PluginProcessor.h"
#include <gtest/gtest.h>
#include <memory>

class PluginProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        juce::MessageManager::getInstance();
        processor = std::make_unique<GravisynthPluginProcessor>();
    }

    void TearDown() override { processor.reset(); }

    std::unique_ptr<GravisynthPluginProcessor> processor;
};

TEST_F(PluginProcessorTest, BasicProperties) {
    EXPECT_EQ(processor->getName(), juce::String("Gravisynth"));
    EXPECT_TRUE(processor->acceptsMidi());
    EXPECT_FALSE(processor->producesMidi());
    EXPECT_TRUE(processor->hasEditor());
    EXPECT_EQ(processor->getNumPrograms(), 1);
}

TEST_F(PluginProcessorTest, PrepareToPlayConfiguresGraph) {
    processor->prepareToPlay(44100.0, 512);
    auto& graph = processor->getGraph();
    EXPECT_GT(graph.getNumNodes(), 0);
    processor->releaseResources();
}

TEST_F(PluginProcessorTest, ProcessBlockProducesOutput) {
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;

    processor->processBlock(buffer, midi);

    EXPECT_EQ(buffer.getNumChannels(), 2);
    EXPECT_EQ(buffer.getNumSamples(), 512);

    processor->releaseResources();
}

TEST_F(PluginProcessorTest, StateRoundTrip) {
    processor->prepareToPlay(44100.0, 512);

    juce::MemoryBlock stateData;
    processor->getStateInformation(stateData);
    EXPECT_GT(stateData.getSize(), 0u);

    auto jsonString = juce::String::fromUTF8(static_cast<const char*>(stateData.getData()), (int)stateData.getSize());
    auto json = juce::JSON::parse(jsonString);
    EXPECT_TRUE(json.isObject());

    auto processor2 = std::make_unique<GravisynthPluginProcessor>();
    processor2->prepareToPlay(44100.0, 512);
    processor2->setStateInformation(stateData.getData(), (int)stateData.getSize());

    EXPECT_GT(processor2->getGraph().getNumNodes(), 0);

    processor->releaseResources();
    processor2->releaseResources();
}

TEST_F(PluginProcessorTest, ModRoutingOperations) {
    processor->prepareToPlay(44100.0, 512);

    auto routings = processor->getActiveModRoutings();

    int initialRoutings = (int)routings.size();
    processor->addEmptyModRouting();
    routings = processor->getActiveModRoutings();
    EXPECT_EQ((int)routings.size(), initialRoutings + 1);

    if (!routings.empty()) {
        auto lastRouting = routings.back();
        processor->removeModRouting(lastRouting.attenuverterNodeID);
        routings = processor->getActiveModRoutings();
        EXPECT_EQ((int)routings.size(), initialRoutings);
    }

    processor->releaseResources();
}

TEST_F(PluginProcessorTest, SampleRateChange) {
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    juce::MidiBuffer midi;
    processor->processBlock(buffer, midi);

    processor->releaseResources();

    processor->prepareToPlay(96000.0, 1024);

    juce::AudioBuffer<float> buffer2(2, 1024);
    buffer2.clear();
    processor->processBlock(buffer2, midi);

    EXPECT_EQ(buffer2.getNumSamples(), 1024);

    processor->releaseResources();
}

TEST_F(PluginProcessorTest, ZeroBufferSize) {
    processor->prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 0);
    juce::MidiBuffer midi;

    processor->processBlock(buffer, midi);

    processor->releaseResources();
}

TEST_F(PluginProcessorTest, MidiInputNodeExists) {
    auto& graph = processor->getGraph();
    bool hasMidiInput = false;
    for (auto* node : graph.getNodes()) {
        if (auto* iop = dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(node->getProcessor())) {
            if (iop->getType() == juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode) {
                hasMidiInput = true;
                break;
            }
        }
    }
    EXPECT_TRUE(hasMidiInput);
}
