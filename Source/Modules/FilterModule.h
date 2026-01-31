#pragma once

#include "ModuleBase.h"
#include <juce_dsp/juce_dsp.h>

class FilterModule : public ModuleBase {
public:
    FilterModule()
        : ModuleBase("Filter", 2, 1) {
        addParameter(cutoffParam = new juce::AudioParameterFloat("cutoff", "Cutoff", 20.0f, 20000.0f, 440.0f));
        addParameter(resonanceParam = new juce::AudioParameterFloat("resonance", "Resonance", 0.0f, 1.0f, 0.1f));
        addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive", 1.0f, 10.0f, 1.0f));
        addParameter(modAmountParam = new juce::AudioParameterFloat("modAmt", "FM Amount", 0.0f, 1.0f, 1.0f));

        enableVisualBuffer(true);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec = {sampleRate, static_cast<juce::uint32>(samplesPerBlock),
                                       (juce::uint32)getTotalNumInputChannels()};
        ladder.prepare(spec);
        ladder.setMode(juce::dsp::LadderFilterMode::LPF24);
        ladder.setEnabled(true);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) override {

        float baseCutoff = *cutoffParam;
        float res = *resonanceParam;
        float drive = *driveParam;
        float modAmt = *modAmountParam;

        ladder.setResonance(res);
        ladder.setDrive(drive);

        if (buffer.getNumChannels() == 0)
            return;

        bool hasCV = buffer.getNumChannels() > 1;
        const float* cvCh = hasCV ? buffer.getReadPointer(1) : nullptr;
        int numSamples = buffer.getNumSamples();

        juce::dsp::AudioBlock<float> block(buffer);
        auto singleChannelBlock = block.getSingleChannelBlock(0);

        for (int i = 0; i < numSamples; ++i) {
            float mod = 1.0f;
            if (cvCh) {
                float octaves = 4.0f;
                float pitchMod = cvCh[i] * modAmt * octaves;
                mod = std::pow(2.0f, pitchMod);
            }

            float f = baseCutoff * mod;
            f = juce::jlimit(20.0f, 20000.0f, f);

            ladder.setCutoffFrequencyHz(f);

            auto sampleBlock = singleChannelBlock.getSubBlock(i, 1);
            juce::dsp::ProcessContextReplacing<float> context(sampleBlock);
            ladder.process(context);
        }

        // Push to visual buffer
        if (auto* vb = getVisualBuffer()) {
            auto* ch = buffer.getReadPointer(0);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                vb->pushSample(ch[i]);
            }
        }
    }

private:
    juce::dsp::LadderFilter<float> ladder;
    juce::AudioParameterFloat* cutoffParam;
    juce::AudioParameterFloat* resonanceParam;
    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterFloat* modAmountParam;
};
