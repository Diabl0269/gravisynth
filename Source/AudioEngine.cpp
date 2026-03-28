#include "AudioEngine.h"
#include "PresetManager.h"

AudioEngine::AudioEngine()
    : mainProcessorGraph() {
    // Register basic formats if needed, though we use internal processors mainly
}

AudioEngine::~AudioEngine() { shutdown(); }

void AudioEngine::initialise() {
    deviceManager.initialiseWithDefaultDevices(0, 2); // 0 inputs, 2 outputs
    deviceManager.addAudioCallback(&processorPlayer);
    processorPlayer.setProcessor(&mainProcessorGraph);

    gsynth::PresetManager::loadDefaultPreset(mainProcessorGraph);
}

void AudioEngine::shutdown() {
    deviceManager.removeAudioCallback(&processorPlayer);
    processorPlayer.setProcessor(nullptr);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device) {
    mainProcessorGraph.prepareToPlay(device->getCurrentSampleRate(), device->getCurrentBufferSizeSamples());
}

void AudioEngine::audioDeviceStopped() { mainProcessorGraph.releaseResources(); }

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                                   float* const* outputChannelData, int numOutputChannels,
                                                   int numSamples, const juce::AudioIODeviceCallbackContext& context) {
    processorPlayer.audioDeviceIOCallbackWithContext(inputChannelData, numInputChannels, outputChannelData,
                                                     numOutputChannels, numSamples, context);
}
