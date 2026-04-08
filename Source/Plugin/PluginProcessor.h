#pragma once

#include "../IGraphManager.h"
#include <juce_audio_processors/juce_audio_processors.h>

class GravisynthPluginProcessor
    : public juce::AudioProcessor
    , public IGraphManager {
public:
    GravisynthPluginProcessor();
    ~GravisynthPluginProcessor() override;

    // juce::AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Gravisynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // IGraphManager implementation
    juce::AudioProcessorGraph& getGraph() override { return processorGraph; }
    std::vector<IGraphManager::ModRoutingInfo> getActiveModRoutings() const override;
    void addModRouting(juce::AudioProcessorGraph::NodeID sourceNodeID, int sourceChannelIndex,
                       juce::AudioProcessorGraph::NodeID destNodeID, int destChannelIndex) override;
    void addEmptyModRouting() override;
    void removeModRouting(juce::AudioProcessorGraph::NodeID attenuverterNodeID) override;
    void toggleModBypass(juce::AudioProcessorGraph::NodeID attenuverterNodeID) override;
    bool isModBypassed(juce::AudioProcessorGraph::NodeID attenuverterNodeID) const override;
    void updateModuleNames() override;

private:
    juce::AudioProcessorGraph processorGraph;

    void ensureMidiInputNode();
    void createDefaultPatch();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GravisynthPluginProcessor)
};
