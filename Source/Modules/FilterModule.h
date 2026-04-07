#pragma once

#include "ModuleBase.h"
#include <atomic>
#include <juce_dsp/juce_dsp.h>

class FilterModule : public ModuleBase {
public:
    FilterModule()
        : ModuleBase("Filter", 4, 1) {
        addParameter(cutoffParam = new juce::AudioParameterFloat("cutoff", "Cutoff", 20.0f, 20000.0f, 440.0f));
        addParameter(resonanceParam = new juce::AudioParameterFloat("resonance", "Resonance", 0.0f, 1.0f, 0.1f));
        addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive", 1.0f, 10.0f, 1.0f));
        addParameter(filterTypeParam = new juce::AudioParameterChoice(
                         "filterType", "Filter Type",
                         juce::StringArray{"LPF24", "LPF12", "HPF24", "HPF12", "BPF24", "BPF12", "Notch"}, 0));

        enableVisualBuffer(true);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec = {sampleRate, static_cast<juce::uint32>(samplesPerBlock),
                                       (juce::uint32)getTotalNumInputChannels()};
        ladder.prepare(spec);
        lastSampleRate = sampleRate;
        applyFilterType(filterTypeParam->getIndex());
        ladder.setEnabled(true);
        svfForNotch.prepare(spec);
        smoothedCutoff.reset(sampleRate, 0.005); // 5ms smoothing
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/) override {
        if (isBypassed())
            return;

        smoothedCutoff.setTargetValue(*cutoffParam);
        applyFilterType(filterTypeParam->getIndex());
        float baseRes = *resonanceParam;
        float baseDrive = *driveParam;

        // Always update atomics with base param values (overridden per-sample if CV active)
        modulatedCutoff.store(*cutoffParam, std::memory_order_relaxed);
        modulatedResonance.store(baseRes, std::memory_order_relaxed);

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
        auto* audioData = buffer.getWritePointer(0);

        // Check if CV channels have actual signal (not just unconnected noise)
        bool cutoffCVActive = false;
        bool resCVActive = false;
        if (cvCutoffCh) {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += cvCutoffCh[i] * cvCutoffCh[i];
            cutoffCVActive = (rms / numSamples) > 1e-6f;
        }
        if (cvResCh) {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += cvResCh[i] * cvResCh[i];
            resCVActive = (rms / numSamples) > 1e-6f;
        }

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
            if (cutoffCVActive)
                modulatedCutoff.store(f, std::memory_order_relaxed);
            ladder.setCutoffFrequencyHz(f);

            // --- Resonance Modulation ---
            float totalResMod = cvResCh ? cvResCh[i] : 0.0f;
            totalResMod = juce::jlimit(-1.0f, 1.0f, totalResMod);
            float res = baseRes + totalResMod;
            res = juce::jlimit(0.0f, 1.0f, res);
            if (resCVActive)
                modulatedResonance.store(res, std::memory_order_relaxed);
            ladder.setResonance(res);

            // --- Drive Modulation ---
            float totalDriveMod = cvDriveCh ? cvDriveCh[i] : 0.0f;
            totalDriveMod = juce::jlimit(-1.0f, 1.0f, totalDriveMod);
            float drive = baseDrive + (totalDriveMod * 9.0f); // 9.0 is the range (10.0 - 1.0)
            drive = juce::jlimit(1.0f, 10.0f, drive);
            ladder.setDrive(drive);

            if (isNotchMode) {
                svfForNotch.setCutoffFrequency(f);
                svfForNotch.setResonance(0.707f + res * 15.0f);
                svfForNotch.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
                float input = audioData[i];
                float filtered = svfForNotch.processSample(0, input);
                audioData[i] = input - filtered;
            } else {
                auto sampleBlock = singleChannelBlock.getSubBlock(i, 1);
                juce::dsp::ProcessContextReplacing<float> context(sampleBlock);
                ladder.process(context);
            }
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
    juce::String getInputPortLabel(int i) const override {
        const juce::String labels[] = {"Audio", "Cutoff", "Resonance", "Drive"};
        return (i >= 0 && i < 4) ? labels[i] : ModuleBase::getInputPortLabel(i);
    }
    juce::String getOutputPortLabel(int) const override { return "Audio"; }
    ModulationCategory getModulationCategory() const override { return ModulationCategory::Filter; }
    ModuleType getModuleType() const override { return ModuleType::Filter; }

    float getCurrentCutoff() const { return modulatedCutoff.load(std::memory_order_relaxed); }
    float getCurrentResonance() const { return *resonanceParam; }
    float getModulatedResonance() const { return modulatedResonance.load(std::memory_order_relaxed); }
    float getCurrentDrive() const { return *driveParam; }
    int getCurrentFilterType() const { return filterTypeParam->getIndex(); }
    double getLastSampleRate() const { return lastSampleRate; }

private:
    void applyFilterType(int typeIndex) {
        if (typeIndex >= 0 && typeIndex <= 5) {
            isNotchMode = false;
            juce::dsp::LadderFilterMode modes[] = {
                juce::dsp::LadderFilterMode::LPF24, juce::dsp::LadderFilterMode::LPF12,
                juce::dsp::LadderFilterMode::HPF24, juce::dsp::LadderFilterMode::HPF12,
                juce::dsp::LadderFilterMode::BPF24, juce::dsp::LadderFilterMode::BPF12};
            ladder.setMode(modes[typeIndex]);
        } else if (typeIndex == 6) {
            isNotchMode = true;
        }
    }

    juce::dsp::LadderFilter<float> ladder;
    juce::dsp::StateVariableTPTFilter<float> svfForNotch;
    bool isNotchMode = false;
    double lastSampleRate = 44100.0;
    juce::SmoothedValue<float> smoothedCutoff;
    juce::AudioParameterFloat* cutoffParam;
    juce::AudioParameterFloat* resonanceParam;
    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterChoice* filterTypeParam;
    std::atomic<float> modulatedCutoff{440.0f};
    std::atomic<float> modulatedResonance{0.1f};
};
