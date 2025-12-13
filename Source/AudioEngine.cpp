#include "AudioEngine.h"
#include "Modules/ADSRModule.h"
#include "Modules/FilterModule.h"
#include "Modules/LFOModule.h"
#include "Modules/OscillatorModule.h"
#include "Modules/SequencerModule.h"
#include "Modules/VCAModule.h"

AudioEngine::AudioEngine() : mainProcessorGraph() {
  // Register basic formats if needed, though we use internal processors mainly
}

AudioEngine::~AudioEngine() { shutdown(); }

void AudioEngine::initialise() {
  deviceManager.initialiseWithDefaultDevices(0, 2); // 0 inputs, 2 outputs
  deviceManager.addAudioCallback(&processorPlayer);
  processorPlayer.setProcessor(&mainProcessorGraph);

  createDefaultPatch();
}

void AudioEngine::shutdown() {
  deviceManager.removeAudioCallback(&processorPlayer);
  processorPlayer.setProcessor(nullptr);
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice *device) {
  mainProcessorGraph.prepareToPlay(device->getCurrentSampleRate(),
                                   device->getCurrentBufferSizeSamples());
}

void AudioEngine::audioDeviceStopped() {
  mainProcessorGraph.releaseResources();
}

void AudioEngine::audioDeviceIOCallbackWithContext(
    const float *const *inputChannelData, int numInputChannels,
    float *const *outputChannelData, int numOutputChannels, int numSamples,
    const juce::AudioIODeviceCallbackContext &context) {
  processorPlayer.audioDeviceIOCallbackWithContext(
      inputChannelData, numInputChannels, outputChannelData, numOutputChannels,
      numSamples, context);
}

void AudioEngine::createDefaultPatch() {
  mainProcessorGraph.clear();

  using AudioGraphIOProcessor =
      juce::AudioProcessorGraph::AudioGraphIOProcessor;

  // Add IO Nodes
  auto inputNode =
      mainProcessorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(
          AudioGraphIOProcessor::audioInputNode));
  auto outputNode =
      mainProcessorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(
          AudioGraphIOProcessor::audioOutputNode));

  // Add Modules
  auto sequencerNode =
      mainProcessorGraph.addNode(std::make_unique<SequencerModule>());
  auto oscillatorNode =
      mainProcessorGraph.addNode(std::make_unique<OscillatorModule>());
  auto filterNode =
      mainProcessorGraph.addNode(std::make_unique<FilterModule>());
  auto vcaNode = mainProcessorGraph.addNode(std::make_unique<VCAModule>());
  auto adsrNode =
      mainProcessorGraph.addNode(std::make_unique<ADSRModule>("Amp Env"));
  auto filterAdsrNode =
      mainProcessorGraph.addNode(std::make_unique<ADSRModule>("Filter Env"));
  auto lfoNode = mainProcessorGraph.addNode(std::make_unique<LFOModule>());

  // Set positions
  // UI auto-layout handles standard modules.
  // We can eventually improve graph editor to read this.

  // Connections
  // Sequencer (Midi) -> Oscillator
  mainProcessorGraph.addConnection(
      {{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {oscillatorNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  // Sequencer (Midi) -> ADSR (Amp)
  mainProcessorGraph.addConnection(
      {{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {adsrNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  // Sequencer (Midi) -> ADSR (Filter)
  mainProcessorGraph.addConnection(
      {{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {filterAdsrNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  // Oscillator (Audio) -> Filter
  mainProcessorGraph.addConnection(
      {{oscillatorNode->nodeID, 0}, {filterNode->nodeID, 0}});

  // Filter (Audio) -> VCA
  mainProcessorGraph.addConnection(
      {{filterNode->nodeID, 0}, {vcaNode->nodeID, 0}}); // VCA Audio In

  // ADSR (Amp) -> VCA (CV)
  mainProcessorGraph.addConnection(
      {{adsrNode->nodeID, 0}, {vcaNode->nodeID, 1}});

  // ADSR (Filter) -> Filter CV
  mainProcessorGraph.addConnection(
      {{filterAdsrNode->nodeID, 0}, {filterNode->nodeID, 1}});

  // Sequencer MIDI to Filter (for CC 74)
  mainProcessorGraph.addConnection(
      {{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
       {filterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

  // VCA -> Output
  mainProcessorGraph.addConnection(
      {{vcaNode->nodeID, 0}, {outputNode->nodeID, 0}});
  mainProcessorGraph.addConnection(
      {{vcaNode->nodeID, 0}, {outputNode->nodeID, 1}});
}
