#pragma once

#include "../ModuleBase.h"
#include <juce_dsp/juce_dsp.h>

class DistortionModule : public ModuleBase {
public:
    DistortionModule()
        : ModuleBase("Distortion", 2, 2) // Stereo FX
    {
        addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive", 1.0f, 10.0f, 1.0f));
        addParameter(mixParam = new juce::AudioParameterFloat("mix", "Mix", 0.0f, 1.0f, 0.5f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // 2x oversampling with polyphase IIR half-band filter
        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // numChannels
            1, // oversampling order (2x)
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);

        oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));

        smoothedMix.reset(sampleRate, 0.005); // 5ms ramp
        smoothedMix.setCurrentAndTargetValue(*mixParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        float drive = *driveParam;
        smoothedMix.setTargetValue(*mixParam);

        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        // Save dry signal
        juce::AudioBuffer<float> dryBuffer;
        dryBuffer.makeCopyOf(buffer);

        // Upsample
        juce::dsp::AudioBlock<float> inputBlock(buffer);
        auto oversampledBlock = oversampling->processSamplesUp(inputBlock);

        applyDistortion(oversampledBlock, drive);
        oversampling->processSamplesDown(inputBlock);
        blendWetDry(buffer, dryBuffer, numChannels, numSamples);
    }

    double getLatencyInSamples() const { return oversampling ? oversampling->getLatencyInSamples() : 0.0; }

private:
    void applyDistortion(juce::dsp::AudioBlock<float>& block, float drive) {
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
            auto* data = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                float input = data[i];
                // Soft clipping using tanh-like function: x / (1 + |x|)
                data[i] = (input * drive) / (1.0f + std::abs(input * drive));
            }
        }
    }

    void blendWetDry(juce::AudioBuffer<float>& wet, const juce::AudioBuffer<float>& dry, int numChannels,
                     int numSamples) {
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* wetData = wet.getWritePointer(ch);
            const auto* dryData = dry.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                float mix = smoothedMix.getNextValue();
                wetData[i] = (wetData[i] * mix) + (dryData[i] * (1.0f - mix));
            }
        }
    }

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterFloat* mixParam;
};
