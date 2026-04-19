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
        addParameter(oversamplingParam = new juce::AudioParameterChoice(
            "oversampling", "Oversampling",
            juce::StringArray{"Off", "2x", "4x"}, 1));  // default 2x preserves backward compat
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // Pre-allocate 2x and 4x oversamplers for real-time safe switching
        oversamplers[0] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);  // 2x
        oversamplers[1] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);  // 4x

        for (auto& os : oversamplers)
            os->initProcessing(static_cast<size_t>(samplesPerBlock));

        dryBuffer.setSize(2, samplesPerBlock);

        smoothedDrive.reset(sampleRate, 0.005);
        smoothedMix.reset(sampleRate, 0.005);
        smoothedDrive.setCurrentAndTargetValue(*driveParam);
        smoothedMix.setCurrentAndTargetValue(*mixParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        if (isBypassed())
            return;

        juce::ignoreUnused(midiMessages);

        int numSamples = buffer.getNumSamples();
        const float* cvDrive = (buffer.getNumChannels() > 2) ? buffer.getReadPointer(2) : nullptr;
        const float* cvMix = (buffer.getNumChannels() > 3) ? buffer.getReadPointer(3) : nullptr;

        smoothedDrive.setTargetValue(*driveParam);
        smoothedMix.setTargetValue(*mixParam);

        // Mix=0: skip all processing (true bypass)
        if (*mixParam <= 0.0f) {
            smoothedDrive.skip(numSamples);
            smoothedMix.skip(numSamples);
            for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
                buffer.clear(ch, 0, numSamples);
            return;
        }

        // Save dry signal (dryBuffer pre-allocated in prepareToPlay)
        for (int ch = 0; ch < 2; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer.getReadPointer(ch), numSamples);

        int oversamplingIndex = oversamplingParam->getIndex();  // 0=Off, 1=2x, 2=4x

        if (oversamplingIndex == 0) {
            // Off: process at 1x rate
            for (int ch = 0; ch < 2; ++ch) {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i) {
                    float driveMod = cvDrive ? cvDrive[i] : 0.0f;
                    float drive = juce::jlimit(1.0f, 20.0f, smoothedDrive.getNextValue() + (driveMod * 10.0f));
                    data[i] = applyWaveshaper(data[i], drive);
                }
            }
        } else {
            // 2x or 4x: upsample, distort, downsample
            auto& os = *oversamplers[oversamplingIndex - 1];  // index 0=2x, 1=4x
            int factor = static_cast<int>(os.getOversamplingFactor());

            juce::dsp::AudioBlock<float> fullBlock(buffer);
            juce::dsp::AudioBlock<float> audioBlock = fullBlock.getSubsetChannelBlock(0, 2);
            auto oversampledBlock = os.processSamplesUp(audioBlock);

            for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch) {
                auto* data = oversampledBlock.getChannelPointer(ch);
                float currentDrive = 1.0f;
                for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i) {
                    int sampleIdx = static_cast<int>(i) / factor;
                    // Advance smoother once per original-rate sample, hold for sub-samples
                    if (i % static_cast<size_t>(factor) == 0) {
                        float driveMod = cvDrive ? cvDrive[std::min(sampleIdx, numSamples - 1)] : 0.0f;
                        currentDrive = juce::jlimit(1.0f, 20.0f, smoothedDrive.getNextValue() + (driveMod * 10.0f));
                    }
                    data[i] = applyWaveshaper(data[i], currentDrive);
                }
            }

            os.processSamplesDown(audioBlock);
        }

        // Wet/dry blend at 1x rate
        for (int ch = 0; ch < 2; ++ch) {
            auto* wetData = buffer.getWritePointer(ch);
            const auto* dryData = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                float mixMod = cvMix ? cvMix[i] : 0.0f;
                float mix = juce::jlimit(0.0f, 1.0f, smoothedMix.getNextValue() + mixMod);
                wetData[i] = (wetData[i] * mix) + (dryData[i] * (1.0f - mix));
            }
        }

        // Clear CV channels
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
            buffer.clear(ch, 0, numSamples);
    }

    juce::String getInputPortLabel(int i) const override {
        const juce::String labels[] = {"Left", "Right", "Drive", "Mix"};
        return (i >= 0 && i < 4) ? labels[i] : ModuleBase::getInputPortLabel(i);
    }
    juce::String getOutputPortLabel(int i) const override { return i == 0 ? "Left" : "Right"; }

    std::vector<ModulationTarget> getModulationTargets() const override { return {{"Drive", 2}, {"Mix", 3}}; }

    ModulationCategory getModulationCategory() const override { return ModulationCategory::FX; }
    ModuleType getModuleType() const override { return ModuleType::Distortion; }

    double getLatencyInSamples() const {
        if (!oversamplingParam) return 0.0;
        int idx = oversamplingParam->getIndex();
        if (idx == 0) return 0.0;
        return oversamplers[idx - 1] ? oversamplers[idx - 1]->getLatencyInSamples() : 0.0;
    }

private:
    // Soft clipper: x*(1+k) / (1+k*|x|), where k = drive-1.
    // At drive=1 (k=0): output = input (transparent, no distortion).
    // At drive=20 (k=19): heavy saturation, peak output stays at 1.0.
    // Smooth transition — no crossfade needed, mix knob has full control.
    static float applyWaveshaper(float input, float drive) {
        float k = drive - 1.0f;
        if (k <= 0.0f) return input;
        return input * (1.0f + k) / (1.0f + k * std::abs(input));
    }

    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[2];  // [0]=2x, [1]=4x
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

    juce::AudioParameterFloat* driveParam = nullptr;
    juce::AudioParameterFloat* mixParam = nullptr;
    juce::AudioParameterChoice* oversamplingParam = nullptr;
};
