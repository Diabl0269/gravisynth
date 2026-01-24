#pragma once

#include "../ModuleBase.h"
#include <juce_audio_basics/juce_audio_basics.h>

class ReverbModule : public ModuleBase {
public:
  ReverbModule() : ModuleBase("Reverb", 2, 2) {
    addParameter(roomSizeParam = new juce::AudioParameterFloat(
                     "roomSize", "Room Size", 0.0f, 1.0f, 0.5f));
    addParameter(dampingParam = new juce::AudioParameterFloat(
                     "damping", "Damping", 0.0f, 1.0f, 0.5f));
    addParameter(wetParam = new juce::AudioParameterFloat("wet", "Wet", 0.0f,
                                                          1.0f, 0.33f));
    addParameter(dryParam = new juce::AudioParameterFloat("dry", "Dry", 0.0f,
                                                          1.0f, 0.4f));
    addParameter(widthParam = new juce::AudioParameterFloat("width", "Width",
                                                            0.0f, 1.0f, 1.0f));
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::ignoreUnused(samplesPerBlock);
    reverb.setSampleRate(sampleRate);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {
    juce::ignoreUnused(midiMessages);

    juce::Reverb::Parameters params;
    params.roomSize = *roomSizeParam;
    params.damping = *dampingParam;
    params.wetLevel = *wetParam;
    params.dryLevel = *dryParam;
    params.width = *widthParam;
    reverb.setParameters(params);

    if (buffer.getNumChannels() >= 2) {
      reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1),
                           buffer.getNumSamples());
    } else {
      reverb.processMono(buffer.getWritePointer(0), buffer.getNumSamples());
    }
  }

private:
  juce::Reverb reverb;
  juce::AudioParameterFloat *roomSizeParam;
  juce::AudioParameterFloat *dampingParam;
  juce::AudioParameterFloat *wetParam;
  juce::AudioParameterFloat *dryParam;
  juce::AudioParameterFloat *widthParam;
};
