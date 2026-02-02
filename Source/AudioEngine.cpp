#include "AudioEngine.h"
#include "Modules/ADSRModule.h"
#include "Modules/FX/DelayModule.h"
#include "Modules/FX/DistortionModule.h"
#include "Modules/FX/ReverbModule.h"
#include "Modules/FilterModule.h"
#include "Modules/LFOModule.h"
#include "Modules/OscillatorModule.h"
#include "Modules/SequencerModule.h"
#include "Modules/VCAModule.h"

AudioEngine::AudioEngine()
    : mainProcessorGraph() {
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

void AudioEngine::createDefaultPatch() {
    mainProcessorGraph.clear();

    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

    // Add IO Nodes
    auto inputNode =
        mainProcessorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    auto outputNode =
        mainProcessorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));

    // Add Modules
    auto sequencerNode = mainProcessorGraph.addNode(std::make_unique<SequencerModule>());
    auto oscillatorNode = mainProcessorGraph.addNode(std::make_unique<OscillatorModule>());
    auto filterNode = mainProcessorGraph.addNode(std::make_unique<FilterModule>());
    auto vcaNode = mainProcessorGraph.addNode(std::make_unique<VCAModule>());
    auto adsrNode = mainProcessorGraph.addNode(std::make_unique<ADSRModule>("Amp Env"));
    auto filterAdsrNode = mainProcessorGraph.addNode(std::make_unique<ADSRModule>("Filter Env"));
    auto lfoNode = mainProcessorGraph.addNode(std::make_unique<LFOModule>());

    auto distortionNode = mainProcessorGraph.addNode(std::make_unique<DistortionModule>());
    auto delayNode = mainProcessorGraph.addNode(std::make_unique<DelayModule>());
    auto reverbNode = mainProcessorGraph.addNode(std::make_unique<ReverbModule>());

    // Set positions
    // UI auto-layout handles standard modules.
    // We can eventually improve graph editor to read this.

    sequencerNode->properties.set("x", 10.0f);
    sequencerNode->properties.set("y", 80.0f);
    oscillatorNode->properties.set("x", 540.0f);
    oscillatorNode->properties.set("y", 50.0f);
    filterNode->properties.set("x", 830.0f);
    filterNode->properties.set("y", 50.0f);
    vcaNode->properties.set("x", 1120.0f);
    vcaNode->properties.set("y", 50.0f);
    adsrNode->properties.set("x", 540.0f);
    adsrNode->properties.set("y", 450.0f);
    filterAdsrNode->properties.set("x", 845.0f);
    filterAdsrNode->properties.set("y", 430.0f);
    lfoNode->properties.set("x", 70.0f);
    lfoNode->properties.set("y", 500.0f);

    distortionNode->properties.set("x", 1410.0f);
    distortionNode->properties.set("y", 50.0f);
    delayNode->properties.set("x", 1690.0f);
    delayNode->properties.set("y", 50.0f);
    reverbNode->properties.set("x", 1970.0f);
    reverbNode->properties.set("y", 50.0f);

    outputNode->properties.set("x", 2250.0f);
    outputNode->properties.set("y", 300.0f);
    inputNode->properties.set("x", 10.0f);
    inputNode->properties.set("y", 10.0f);

    // Connections
    // Sequencer (Midi) -> Oscillator
    mainProcessorGraph.addConnection({{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                                      {oscillatorNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    // Sequencer (Midi) -> ADSR (Amp)
    mainProcessorGraph.addConnection({{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                                      {adsrNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    // Sequencer (Midi) -> ADSR (Filter)
    mainProcessorGraph.addConnection({{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                                      {filterAdsrNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    // Oscillator (Audio) -> Filter
    mainProcessorGraph.addConnection({{oscillatorNode->nodeID, 0}, {filterNode->nodeID, 0}});

    // Filter (Audio) -> VCA
    mainProcessorGraph.addConnection({{filterNode->nodeID, 0}, {vcaNode->nodeID, 0}}); // VCA Audio In

    // ADSR (Amp) -> VCA (CV)
    mainProcessorGraph.addConnection({{adsrNode->nodeID, 0}, {vcaNode->nodeID, 1}});

    // ADSR (Filter) -> Filter CV
    mainProcessorGraph.addConnection({{filterAdsrNode->nodeID, 0}, {filterNode->nodeID, 1}});

    // Sequencer MIDI to Filter (for CC 74)
    mainProcessorGraph.addConnection({{sequencerNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                                      {filterNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});

    // VCA -> Distortion
    mainProcessorGraph.addConnection({{vcaNode->nodeID, 0}, {distortionNode->nodeID, 0}});
    mainProcessorGraph.addConnection({{vcaNode->nodeID, 0}, {distortionNode->nodeID, 1}});

    // Distortion -> Delay
    mainProcessorGraph.addConnection({{distortionNode->nodeID, 0}, {delayNode->nodeID, 0}});
    mainProcessorGraph.addConnection({{distortionNode->nodeID, 1}, {delayNode->nodeID, 1}});

    // Delay -> Reverb
    mainProcessorGraph.addConnection({{delayNode->nodeID, 0}, {reverbNode->nodeID, 0}});
    mainProcessorGraph.addConnection({{delayNode->nodeID, 1}, {reverbNode->nodeID, 1}});

    // Reverb -> Output
    mainProcessorGraph.addConnection({{reverbNode->nodeID, 0}, {outputNode->nodeID, 0}});
    mainProcessorGraph.addConnection({{reverbNode->nodeID, 1}, {outputNode->nodeID, 1}});
}
