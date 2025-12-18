#pragma once

#include "../ModuleBase.h"

class DistortionModule : public ModuleBase {
public:
  DistortionModule()
      : ModuleBase("Distortion", 2, 2) // Stereo FX
  {
    addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive",
                                                            1.0f, 10.0f, 1.0f));
    addParameter(mixParam = new juce::AudioParameterFloat("mix", "Mix", 0.0f,
                                                          1.0f, 0.5f));
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {
    juce::ignoreUnused(midiMessages);

    float drive = *driveParam;
    float mix = *mixParam;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
      auto *data = buffer.getWritePointer(ch);
      for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float input = data[i];

        // Soft clipping using tanh-like function: x / (1 + |x|)
        float distorted = (input * drive) / (1.0f + std::abs(input * drive));

        // Mix
        data[i] = (distorted * mix) + (input * (1.0f - mix));
      }
    }
  }

private:
  juce::AudioParameterFloat *driveParam;
  juce::AudioParameterFloat *mixParam;
};
