#pragma once

#include "ModuleBase.h"
#include <juce_dsp/juce_dsp.h>

class FilterModule : public ModuleBase {
public:
    FilterModule()
        : ModuleBase("Filter", 4, 1) {
        addParameter(cutoffParam = new juce::AudioParameterFloat("cutoff", "Cutoff", 20.0f, 20000.0f, 440.0f));
        addParameter(resonanceParam = new juce::AudioParameterFloat("resonance", "Resonance", 0.0f, 1.0f, 0.1f));
        addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive", 1.0f, 10.0f, 1.0f));

        enableVisualBuffer(true);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec = {sampleRate, static_cast<juce::uint32>(samplesPerBlock),
                                       (juce::uint32)getTotalNumInputChannels()};
        ladder.prepare(spec);
        ladder.setMode(juce::dsp::LadderFilterMode::LPF24);
        ladder.setEnabled(true);
        smoothedCutoff.reset(sampleRate, 0.005); // 5ms smoothing
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) override {
        smoothedCutoff.setTargetValue(*cutoffParam);
        float baseRes = *resonanceParam;
        float baseDrive = *driveParam;

        if (buffer.getNumChannels() == 0)
            return;

        bool hasCutoffCV = buffer.getNumChannels() > 1;
        bool hasResCV = buffer.getNumChannels() > 2;
        bool hasDriveCV = buffer.getNumChannels() > 3;

        const float* cvCutoffCh = hasCutoffCV ? buffer.getReadPointer(1) : nullptr;
        const float* cvResCh = hasResCV ? buffer.getReadPointer(2) : nullptr;
        const float* cvDriveCh = hasDriveCV ? buffer.getReadPointer(3) : nullptr;

        int numSamples = buffer.getNumSamples();

        juce::dsp::AudioBlock<float> block(buffer);
        auto singleChannelBlock = block.getSingleChannelBlock(0);

        for (int i = 0; i < numSamples; ++i) {
            float baseCutoff = smoothedCutoff.getNextValue();

            // --- Cutoff Modulation ---
            float totalCutoffMod = cvCutoffCh ? cvCutoffCh[i] : 0.0f;
            totalCutoffMod = juce::jlimit(-1.0f, 1.0f, totalCutoffMod);

            float f = baseCutoff;
            if (totalCutoffMod > 0.0f) {
                f = baseCutoff * std::pow(20000.0f / baseCutoff, totalCutoffMod);
            } else if (totalCutoffMod < 0.0f) {
                f = baseCutoff * std::pow(20.0f / baseCutoff, -totalCutoffMod);
            }
            f = juce::jlimit(20.0f, 20000.0f, f);
            ladder.setCutoffFrequencyHz(f);

            // --- Resonance Modulation ---
            float totalResMod = cvResCh ? cvResCh[i] : 0.0f;
            totalResMod = juce::jlimit(-1.0f, 1.0f, totalResMod);
            float res = baseRes + totalResMod;
            res = juce::jlimit(0.0f, 1.0f, res);
            ladder.setResonance(res);

            // --- Drive Modulation ---
            float totalDriveMod = cvDriveCh ? cvDriveCh[i] : 0.0f;
            totalDriveMod = juce::jlimit(-1.0f, 1.0f, totalDriveMod);
            float drive = baseDrive + (totalDriveMod * 9.0f); // 9.0 is the range (10.0 - 1.0)
            drive = juce::jlimit(1.0f, 10.0f, drive);
            ladder.setDrive(drive);

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

    std::vector<ModulationTarget> getModulationTargets() const override {
        return {{"Cutoff", 1}, {"Resonance", 2}, {"Drive", 3}};
    }
    ModulationCategory getModulationCategory() const override { return ModulationCategory::Filter; }
    ModuleType getModuleType() const override { return ModuleType::Filter; }

private:
    juce::dsp::LadderFilter<float> ladder;
    juce::SmoothedValue<float> smoothedCutoff;
    juce::AudioParameterFloat* cutoffParam;
    juce::AudioParameterFloat* resonanceParam;
    juce::AudioParameterFloat* driveParam;
};
