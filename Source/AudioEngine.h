#pragma once

#include "IGraphManager.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>

class AudioEngine
    : public juce::AudioIODeviceCallback
    , public IGraphManager {
public:
    AudioEngine();
    ~AudioEngine() override;

    void initialise();
    void shutdown();

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels, int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::AudioProcessorGraph& getGraph() override { return mainProcessorGraph; }
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    std::vector<IGraphManager::ModRoutingInfo> getActiveModRoutings() const override;
    void addModRouting(juce::AudioProcessorGraph::NodeID sourceNodeID, int sourceChannelIndex,
                       juce::AudioProcessorGraph::NodeID destNodeID, int destChannelIndex) override;
    void addEmptyModRouting() override;
    void removeModRouting(juce::AudioProcessorGraph::NodeID attenuverterNodeID) override;
    void toggleModBypass(juce::AudioProcessorGraph::NodeID attenuverterNodeID) override;
    bool isModBypassed(juce::AudioProcessorGraph::NodeID attenuverterNodeID) const override;
    void updateModuleNames() override;

private:
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorGraph mainProcessorGraph;
    juce::AudioProcessorPlayer processorPlayer;

    void createDefaultPatch();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
