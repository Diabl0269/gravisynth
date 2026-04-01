#pragma once

#include "ModuleBase.h"

class AttenuverterModule : public ModuleBase {
public:
    AttenuverterModule()
        : ModuleBase("Attenuverter", 2, 1) // 1 In (Signal), 1 In (CV Amount), 1 Out
    {
        addParameter(amountParam = new juce::AudioParameterFloat("amount", "Amount", -1.0f, 1.0f, 0.0f));
        addParameter(bypassParam = new juce::AudioParameterBool("bypass", "Bypass", false));
    }

    ModulationCategory getModulationCategory() const override { return ModulationCategory::Other; }
    std::vector<ModulationTarget> getModulationTargets() const override { return {{"Amount", 1}}; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        smoothedAmount.reset(sampleRate, 0.01);
        smoothedAmount.setCurrentAndTargetValue(*amountParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        if (buffer.getNumChannels() == 0)
            return;

        int numSamples = buffer.getNumSamples();
        if (bypassParam->get()) {
            buffer.clear(0, 0, numSamples);
            return;
        }

        auto* audioData = buffer.getWritePointer(0);
        float baseAmount = amountParam->get();
        smoothedAmount.setTargetValue(baseAmount);

        // Get CV from second channel if available (Sidechain/Modulation of Amount)
        const float* cvAmount = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;

        for (int sample = 0; sample < numSamples; ++sample) {
            float currentAmount = smoothedAmount.getNextValue();

            // Additive Modulation (Safe for unconnected inputs where cvAmount = 0.0)
            if (cvAmount) {
                currentAmount += cvAmount[sample];
                currentAmount = juce::jlimit(-1.0f, 1.0f, currentAmount);
            }

            audioData[sample] *= currentAmount;
        }

        // Output mono on channel 0, clear other channels to prevent leakage
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, numSamples);
        }
    }

private:
    juce::AudioParameterFloat* amountParam;
    juce::AudioParameterBool* bypassParam;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
};
