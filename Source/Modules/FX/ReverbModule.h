#pragma once

#include "../ModuleBase.h"
#include <juce_audio_basics/juce_audio_basics.h>

class ReverbModule : public ModuleBase {
public:
    ReverbModule()
        : ModuleBase("Reverb", 7, 2) { // 2 Audio + 5 CV (Size, Damp, Wet, Dry, Width)
        addParameter(roomSizeParam = new juce::AudioParameterFloat("roomSize", "Room Size", 0.0f, 1.0f, 0.5f));
        addParameter(dampingParam = new juce::AudioParameterFloat("damping", "Damping", 0.0f, 1.0f, 0.5f));
        addParameter(wetParam = new juce::AudioParameterFloat("wet", "Wet", 0.0f, 1.0f, 0.33f));
        addParameter(dryParam = new juce::AudioParameterFloat("dry", "Dry", 0.0f, 1.0f, 0.4f));
        addParameter(widthParam = new juce::AudioParameterFloat("width", "Width", 0.0f, 1.0f, 1.0f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        reverb.setSampleRate(sampleRate);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        int numSamples = buffer.getNumSamples();
        const float* cvSize = (buffer.getNumChannels() > 2) ? buffer.getReadPointer(2) : nullptr;
        const float* cvDamping = (buffer.getNumChannels() > 3) ? buffer.getReadPointer(3) : nullptr;
        const float* cvWet = (buffer.getNumChannels() > 4) ? buffer.getReadPointer(4) : nullptr;
        const float* cvDry = (buffer.getNumChannels() > 5) ? buffer.getReadPointer(5) : nullptr;
        const float* cvWidth = (buffer.getNumChannels() > 6) ? buffer.getReadPointer(6) : nullptr;

        juce::Reverb::Parameters params;
        params.roomSize = juce::jlimit(0.0f, 1.0f, static_cast<float>(*roomSizeParam) + (cvSize ? cvSize[0] : 0.0f));
        params.damping =
            juce::jlimit(0.0f, 1.0f, static_cast<float>(*dampingParam) + (cvDamping ? cvDamping[0] : 0.0f));
        params.wetLevel = juce::jlimit(0.0f, 1.0f, static_cast<float>(*wetParam) + (cvWet ? cvWet[0] : 0.0f));
        params.dryLevel = juce::jlimit(0.0f, 1.0f, static_cast<float>(*dryParam) + (cvDry ? cvDry[0] : 0.0f));
        params.width = juce::jlimit(0.0f, 1.0f, static_cast<float>(*widthParam) + (cvWidth ? cvWidth[0] : 0.0f));
        reverb.setParameters(params);

        if (buffer.getNumChannels() >= 2) {
            reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
        } else {
            reverb.processMono(buffer.getWritePointer(0), numSamples);
        }

        // Clear CV channels
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, numSamples);
        }
    }

    std::vector<ModulationTarget> getModulationTargets() const override {
        return {{"Size", 2}, {"Damping", 3}, {"Wet", 4}, {"Dry", 5}, {"Width", 6}};
    }

    ModulationCategory getModulationCategory() const override { return ModulationCategory::FX; }

private:
    juce::Reverb reverb;
    juce::AudioParameterFloat* roomSizeParam;
    juce::AudioParameterFloat* dampingParam;
    juce::AudioParameterFloat* wetParam;
    juce::AudioParameterFloat* dryParam;
    juce::AudioParameterFloat* widthParam;
};
