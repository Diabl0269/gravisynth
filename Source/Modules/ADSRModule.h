#pragma once

#include "ModuleBase.h"
#include <algorithm>

class ADSRModule : public ModuleBase {
public:
  ADSRModule(const juce::String &name = "ADSR")
      : ModuleBase(name, 1, 1) // 1 input (gate), 1 output (envelope)
  {
    addParameter(attackParam = new juce::AudioParameterFloat(
                     "attack", "Attack", 0.01f, 5.0f, 0.05f));
    addParameter(decayParam = new juce::AudioParameterFloat("decay", "Decay",
                                                            0.01f, 5.0f, 0.2f));
    addParameter(sustainParam = new juce::AudioParameterFloat(
                     "sustain", "Sustain", 0.0f, 1.0f, 0.0f));
    addParameter(releaseParam = new juce::AudioParameterFloat(
                     "release", "Release", 0.01f, 5.0f, 0.1f));
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::ignoreUnused(samplesPerBlock);
    adsr.setSampleRate(sampleRate);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {
    // Simple MIDI gate handling
    for (const auto metadata : midiMessages) {
      auto message = metadata.getMessage();
      if (message.isNoteOn())
        adsr.noteOn();
      else if (message.isNoteOff())
        adsr.noteOff();
    }

    adsrParams.attack = *attackParam;
    adsrParams.decay = *decayParam;
    adsrParams.sustain = *sustainParam;
    adsrParams.release = *releaseParam;
    adsr.setParameters(adsrParams);

    // Generate valid control signal by filling buffer with 1.0f
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      auto *data = buffer.getWritePointer(ch);
      std::fill(data, data + buffer.getNumSamples(), 1.0f);
    }

    adsr.applyEnvelopeToBuffer(buffer, 0, buffer.getNumSamples());
  }

private:
  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;

  juce::AudioParameterFloat *attackParam;
  juce::AudioParameterFloat *decayParam;
  juce::AudioParameterFloat *sustainParam;
  juce::AudioParameterFloat *releaseParam;
};
