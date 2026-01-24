#pragma once

#include "ModuleBase.h"

class VCAModule : public ModuleBase {
public:
  VCAModule()
      : ModuleBase("VCA", 2, 1) // 1 Audio In, 1 CV In (from ADSR?), 1 Output
  {
    addParameter(gainParam = new juce::AudioParameterFloat("gain", "Gain", 0.0f,
                                                           1.0f, 0.5f));
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {
    juce::ignoreUnused(midiMessages);

    auto totalNumInputChannels = getTotalNumInputChannels();
    // auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Expect Input 0: Audio, Input 1: CV (Optional)
    // But JUCE AudioProcessor inputs are just channels on the main bus usually
    // unless configured specifically. Our ModuleBase sets up Stereo Input (2
    // chans). Let's assume: Channel 0 is Audio (Mono L), Channel 1 is CV (Mono
    // R or 2nd input).

    // This is tricky with stereo buses.
    // Ideally we'd have 2 buses. But let's assume standard stereo interleave.
    // Signal flow: Left = Audio, Right = CV?

    // BETTER APPROACH for Modular Graph:
    // Everything works in stereo pairs?
    // Let's assume the AudioEngine connection logic maps Output 0 -> Input 0.
    // In AudioEngine.cpp we did:
    // Filter:0 -> VCA:0 (Audio)
    // ADSR:0   -> VCA:1 (CV)   <-- This connects to channel 1 of VCA input bus.

    // Safe channel access
    auto *audioData =
        (totalNumInputChannels > 0) ? buffer.getWritePointer(0) : nullptr;
    const float *cvData =
        (totalNumInputChannels > 1) ? buffer.getReadPointer(1) : nullptr;

    if (audioData == nullptr)
      return; // Nothing to process

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      float cv = 1.0f;
      if (cvData != nullptr) {
        cv = cvData[sample];
      }

      // Apply gain and CV
      audioData[sample] = audioData[sample] * (*gainParam) * cv;
    }

    // Copy mono L to R for output if needed
    if (buffer.getNumChannels() > 1) {
      auto *rightData = buffer.getWritePointer(1);
      for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        rightData[sample] = audioData[sample];
    }
  }

private:
  juce::AudioParameterFloat *gainParam;
};
