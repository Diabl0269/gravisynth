#pragma once

#include "ModuleBase.h"

class AttenuverterModule : public ModuleBase {
public:
    AttenuverterModule()
        : ModuleBase("Attenuverter", 1, 1) // 1 In, 1 Out
    {
        addParameter(amountParam = new juce::AudioParameterFloat("amount", "Amount", -1.0f, 1.0f, 0.0f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        smoothedAmount.reset(sampleRate, 0.01);
        smoothedAmount.setCurrentAndTargetValue(*amountParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        if (buffer.getNumChannels() == 0)
            return;

        auto* audioData = buffer.getWritePointer(0);
        smoothedAmount.setTargetValue(*amountParam);
        int numSamples = buffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample) {
            audioData[sample] *= smoothedAmount.getNextValue();
        }

        // Copy channel 0 to other channels if needed (though usually CV is mono)
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch) {
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
        }
    }

    std::vector<ModulationTarget> getModulationTargets() const override { return {{"Amount", 1}}; }

private:
    juce::AudioParameterFloat* amountParam;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
};
