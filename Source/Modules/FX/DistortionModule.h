#pragma once

#include "../ModuleBase.h"
#include <juce_dsp/juce_dsp.h>

class DistortionModule : public ModuleBase {
public:
    DistortionModule()
        : ModuleBase("Distortion", 4, 2) // 2 Audio + 2 CV (Drive, Mix)
    {
        addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive", 1.0f, 20.0f, 1.0f));
        addParameter(mixParam = new juce::AudioParameterFloat("mix", "Mix", 0.0f, 1.0f, 0.5f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // 2x oversampling with polyphase IIR half-band filter
        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // numChannels
            1, // oversampling order (2x)
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);

        oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));

        smoothedDrive.reset(sampleRate, 0.005);
        smoothedMix.reset(sampleRate, 0.005);
        smoothedDrive.setCurrentAndTargetValue(*driveParam);
        smoothedMix.setCurrentAndTargetValue(*mixParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        int numSamples = buffer.getNumSamples();
        const float* cvDrive = (buffer.getNumChannels() > 2) ? buffer.getReadPointer(2) : nullptr;
        const float* cvMix = (buffer.getNumChannels() > 3) ? buffer.getReadPointer(3) : nullptr;

        smoothedDrive.setTargetValue(*driveParam);
        smoothedMix.setTargetValue(*mixParam);

        int numChannels = 2; // Stereo audio processing

        // Save dry signal
        juce::AudioBuffer<float> dryBuffer;
        dryBuffer.setSize(numChannels, numSamples);
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), numSamples);

        // Upsample
        juce::dsp::AudioBlock<float> fullBlock(buffer);
        juce::dsp::AudioBlock<float> audioBlock = fullBlock.getSubsetChannelBlock(0, 2);
        auto oversampledBlock = oversampling->processSamplesUp(audioBlock);

        // Apply distortion with CV
        for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch) {
            auto* data = oversampledBlock.getChannelPointer(ch);
            for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i) {
                // For simplicity, we sample CV at the block level or first sample if high rate
                // But let's just use the current smoothed values + CV
                int sampleIdx = (int)i / (int)oversampling->getOversamplingFactor();
                float driveMod = cvDrive ? cvDrive[std::min(sampleIdx, numSamples - 1)] : 0.0f;

                float drive = juce::jlimit(1.0f, 20.0f, smoothedDrive.getNextValue() + (driveMod * 10.0f));

                float input = data[i];
                // Soft clipping using tanh-like function: x / (1 + |x|)
                data[i] = (input * drive) / (1.0f + std::abs(input * drive));

                // We'll blend after downsampling for better quality
            }
        }

        oversampling->processSamplesDown(audioBlock);

        // BlendWetDry
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* wetData = buffer.getWritePointer(ch);
            const auto* dryData = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                float mixMod = cvMix ? cvMix[i] : 0.0f;
                float mix = juce::jlimit(0.0f, 1.0f, smoothedMix.getCurrentValue() + mixMod);
                wetData[i] = (wetData[i] * mix) + (dryData[i] * (1.0f - mix));
            }
        }

        // Clear CV channels
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, numSamples);
        }
    }

    std::vector<ModulationTarget> getModulationTargets() const override { return {{"Drive", 2}, {"Mix", 3}}; }

    ModulationCategory getModulationCategory() const override { return ModulationCategory::FX; }
    ModuleType getModuleType() const override { return ModuleType::Distortion; }

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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterFloat* mixParam;
};
