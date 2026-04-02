#pragma once

#include "ModuleBase.h"

class AttenuverterModule : public ModuleBase {
public:
    AttenuverterModule()
        : ModuleBase("Attenuverter", 2, 1) // 1 Audio In + 1 CV In (Amount), 1 Out
    {
        addParameter(amountParam = new juce::AudioParameterFloat("amount", "Amount", -1.0f, 1.0f, 0.0f));
        addParameter(bypassParam = new juce::AudioParameterBool("bypass", "Bypass", false));
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

        if (bypassParam->get()) {
            buffer.clear();
            return;
        }

        auto* audioData = buffer.getWritePointer(0);
        auto* cvAmountData = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
        smoothedAmount.setTargetValue(*amountParam);
        int numSamples = buffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample) {
            float amount = smoothedAmount.getNextValue();
            if (cvAmountData)
                amount = juce::jlimit(-1.0f, 1.0f, amount + cvAmountData[sample]);
            audioData[sample] *= amount;
        }

        // Clear CV channel and copy audio to remaining channels
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch) {
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
        }
    }

    std::vector<ModulationTarget> getModulationTargets() const override { return {{"Amount", 1}}; }
    ModuleType getModuleType() const override { return ModuleType::Attenuverter; }

private:
    juce::AudioParameterFloat* amountParam;
    juce::AudioParameterBool* bypassParam;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
};
