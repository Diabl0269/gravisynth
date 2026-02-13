#pragma once

#include "ModuleBase.h"

class VCAModule : public ModuleBase {
public:
    VCAModule()
        : ModuleBase("VCA", 2, 1) // 1 Audio In, 1 CV In (from ADSR?), 1 Output
    {
        addParameter(gainParam = new juce::AudioParameterFloat("gain", "Gain", 0.0f, 1.0f, 0.5f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        smoothedGain.reset(sampleRate, 0.005);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        auto totalNumInputChannels = getTotalNumInputChannels();

        auto* audioData = (totalNumInputChannels > 0) ? buffer.getWritePointer(0) : nullptr;
        const float* cvData = (totalNumInputChannels > 1) ? buffer.getReadPointer(1) : nullptr;

        if (audioData == nullptr)
            return;

        smoothedGain.setTargetValue(*gainParam);
        int numSamples = buffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample) {
            float cv = (cvData != nullptr) ? cvData[sample] : 1.0f;
            audioData[sample] *= smoothedGain.getNextValue() * cv;
        }

        if (buffer.getNumChannels() > 1) {
            auto* rightData = buffer.getWritePointer(1);
            buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
        }
    }

private:
    juce::AudioParameterFloat* gainParam;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedGain;
};
