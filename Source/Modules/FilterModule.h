#pragma once

#include "ModuleBase.h"

class FilterModule : public ModuleBase {
public:
  FilterModule() : ModuleBase("Filter", 2, 1) {
    addParameter(cutoffParam = new juce::AudioParameterFloat(
                     "cutoff", "Cutoff", 20.0f, 20000.0f, 80.0f));
    addParameter(resonanceParam = new juce::AudioParameterFloat(
                     "resonance", "Resonance", 0.0f, 1.0f, 0.5f));
    addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive",
                                                            1.0f, 10.0f, 2.0f));
    addParameter(modAmountParam = new juce::AudioParameterFloat(
                     "modAmount", "FM Amount", 0.0f, 1.0f, 1.0f));
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::dsp::ProcessSpec spec = {sampleRate,
                                   static_cast<juce::uint32>(samplesPerBlock),
                                   (juce::uint32)getTotalNumInputChannels()};
    filter.prepare(spec);
    filter.setMode(juce::dsp::LadderFilterMode::LPF24);
    filter.setEnabled(true);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {

    // MIDI CC 74 can still control it if we want, or just rely on parameter
    for (const auto metadata : midiMessages) {
      auto msg = metadata.getMessage();
      if (msg.isController() && msg.getControllerNumber() == 74) {
        // Optional: map CC to parameter?
        // *modAmountParam = msg.getControllerValue() / 127.0f;
        // Let's leave CC out for now to avoid confusion/overwriting
      }
    }

    float baseCutoff = *cutoffParam;
    float res = *resonanceParam;
    float drive = *driveParam;
    float modAmt = *modAmountParam;

    filter.setResonance(res);
    filter.setDrive(drive);

    bool hasCV = buffer.getNumChannels() > 1;
    // We need to process in-place on the buffer using block context
    // But we need per-sample modulation.
    // We will iterate and create 1-sample sub-blocks.
    // This is overhead-heavy but correct for audio-rate ladder modulation.

    auto *audioCh = buffer.getWritePointer(0);
    const float *cvCh = hasCV ? buffer.getReadPointer(1) : nullptr;
    int numSamples = buffer.getNumSamples();

    juce::dsp::AudioBlock<float> block(buffer);
    // Ladder filter processes all channels in the block.
    // Our graph is managing channels manually, but AudioBlock wrapping 'buffer'
    // includes all channels. We only want to process Channel 0 (Audio). Channel
    // 1 is CV, we don't want to filter it? Or Ladder ignores it if we give it a
    // 1-channel block? Let's create a single-channel block wrapper for Channel
    // 0.
    auto singleChannelBlock = block.getSingleChannelBlock(0);

    for (int i = 0; i < numSamples; ++i) {
      float mod = 0.0f;
      if (cvCh) {
        // Exponential FM
        float octaves = 4.0f;
        float pitchMod = cvCh[i] * modAmt * octaves;
        mod = std::pow(2.0f, pitchMod);
      } else {
        mod = 1.0f;
      }

      float f = baseCutoff * mod;
      f = juce::jlimit(20.0f, 20000.0f, f);

      filter.setCutoffFrequencyHz(f);

      // Process 1 sample
      auto sampleBlock = singleChannelBlock.getSubBlock(i, 1);
      juce::dsp::ProcessContextReplacing<float> context(sampleBlock);
      filter.process(context);
    }
  }

private:
  juce::dsp::LadderFilter<float> filter;
  juce::AudioParameterFloat *cutoffParam;
  juce::AudioParameterFloat *resonanceParam;
  juce::AudioParameterFloat *driveParam;
  juce::AudioParameterFloat *modAmountParam;
};
