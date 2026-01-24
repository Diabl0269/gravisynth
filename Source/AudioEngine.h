#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>

class AudioEngine : public juce::AudioIODeviceCallback {
public:
  AudioEngine();
  ~AudioEngine() override;

  void initialise();
  void shutdown();

  void audioDeviceIOCallbackWithContext(
      const float *const *inputChannelData, int numInputChannels,
      float *const *outputChannelData, int numOutputChannels, int numSamples,
      const juce::AudioIODeviceCallbackContext &context) override;

  void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
  void audioDeviceStopped() override;

  juce::AudioProcessorGraph &getGraph() { return mainProcessorGraph; }
  juce::AudioDeviceManager &getDeviceManager() { return deviceManager; }

private:
  juce::AudioDeviceManager deviceManager;
  juce::AudioProcessorGraph mainProcessorGraph;
  juce::AudioProcessorPlayer processorPlayer;

  void createDefaultPatch();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
