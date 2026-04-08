#pragma once

#include "ModuleBase.h"

class VCAModule : public ModuleBase {
public:
    VCAModule()
        : ModuleBase("VCA", 16, 8) // 0-7: per-voice audio, 8-15: per-voice CV, outputs 0-7: gated audio
    {
        addParameter(gainParam = new juce::AudioParameterFloat("gain", "Gain", 0.0f, 1.0f, 0.5f));
        addParameter(polyParam = new juce::AudioParameterBool("poly", "Poly", false));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        smoothedGain.reset(sampleRate, 0.01);
        smoothedGain.setCurrentAndTargetValue(*gainParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();
        if (numChannels == 0 || numSamples == 0)
            return;

        smoothedGain.setTargetValue(*gainParam);

        if (!polyParam->get()) {
            // --- Mono mode: read CV from ch1 (legacy) or ch8 (poly layout fallback) ---
            auto* audioData = buffer.getWritePointer(0);
            const float* cvData = (numChannels > 1) ? buffer.getReadPointer(1) : nullptr;

            // If ch1 has no signal, try ch8 (poly envelope routing)
            if (cvData != nullptr && numChannels > 8) {
                float rms1 = 0.0f;
                for (int s = 0; s < std::min(numSamples, 64); ++s)
                    rms1 += cvData[s] * cvData[s];
                if (rms1 < 1e-6f)
                    cvData = buffer.getReadPointer(8);
            }

            for (int s = 0; s < numSamples; ++s) {
                float cv = (cvData != nullptr) ? cvData[s] : 1.0f;
                audioData[s] *= smoothedGain.getNextValue() * cv;
            }

            if (numChannels > 1) {
                buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
            }
        } else {
            // --- Poly mode: 8 voices ---
            for (int s = 0; s < numSamples; ++s) {
                float gain = smoothedGain.getNextValue();
                for (int v = 0; v < MAX_VOICES && v < numChannels; ++v) {
                    float audio = buffer.getSample(v, s);
                    float cv = (v + MAX_VOICES < numChannels) ? buffer.getSample(v + MAX_VOICES, s) : 1.0f;
                    buffer.setSample(v, s, audio * gain * cv);
                }
            }
            // No stereo copy in poly mode
        }
    }

    std::vector<ModulationTarget> getModulationTargets() const override { return {{"CV", 1}}; }
    juce::String getInputPortLabel(int i) const override { return (i == 0) ? "Audio" : "CV"; }
    juce::String getOutputPortLabel(int) const override { return "Audio"; }
    int getVisibleInputPortCount() const override { return 2; }
    int getVisibleOutputPortCount() const override { return 1; }
    ModuleType getModuleType() const override { return ModuleType::VCA; }

private:
    static constexpr int MAX_VOICES = 8;
    juce::AudioParameterFloat* gainParam = nullptr;
    juce::AudioParameterBool* polyParam = nullptr;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;
};
