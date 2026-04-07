#pragma once

#include "ModuleBase.h"
#include <cmath>

class OscillatorModule : public ModuleBase {
public:
    OscillatorModule()
        : ModuleBase("Oscillator", 6, 1) // 1 pitch mod, 5 param mod inputs, 1 output
    {
        addParameter(waveformParam = new juce::AudioParameterChoice("waveform", "Waveform",
                                                                    {"Sine", "Square", "Saw", "Triangle"}, 0));
        addParameter(octaveParam = new juce::AudioParameterInt(juce::ParameterID("octave", 1), "Octave", -4, 4, 0));
        addParameter(coarseParam = new juce::AudioParameterInt(juce::ParameterID("coarse", 1), "Coarse", -12, 12, 0));
        addParameter(fineParam = new juce::AudioParameterFloat("fine", "Fine", -100.0f, 100.0f, 0.0f));
        addParameter(levelParam = new juce::AudioParameterFloat("level", "Level", 0.0f, 1.0f, 1.0f));

        enableVisualBuffer(true);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        currentSampleRate = sampleRate;
        smoothedFreq.reset(sampleRate, 0.005); // 5ms glide
        // Initialize to default frequency to avoid pitch sweep from 0 on first processBlock
        float initPitch =
            lastMidiNote + (octaveParam->get() * 12.0f) + (float)coarseParam->get() + (fineParam->get() / 100.0f);
        float initFreq = 440.0f * std::pow(2.0f, (initPitch - 69.0f) / 12.0f);
        smoothedFreq.setCurrentAndTargetValue(initFreq);
        debugLogCounter = 0;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        if (isBypassed()) {
            buffer.clear();
            return;
        }

        // Process MIDI for Pitch
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn()) {
                lastMidiNote = (float)msg.getNoteNumber();
            }
        }

        if (buffer.getNumChannels() == 0)
            return;

        // Clear input-only CV channels (1+) to prevent JUCE buffer pool garbage
        // being misread as CV modulation. Connected sources refill before processBlock.
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, buffer.getNumSamples());
        }

        float totalPitch =
            lastMidiNote + (octaveParam->get() * 12.0f) + (float)coarseParam->get() + (fineParam->get() / 100.0f);
        float targetFreq = 440.0f * std::pow(2.0f, (totalPitch - 69.0f) / 12.0f);
        smoothedFreq.setTargetValue(targetFreq);

        int baseWaveform = waveformParam->getIndex();
        int numSamples = buffer.getNumSamples();

        // Clear input-only CV channels (1+) to prevent JUCE buffer pool garbage
        // being misread as CV modulation. Connected sources refill before processBlock.
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, numSamples);
        }

        // Channel 0 is shared between CV pitch input and audio output.
        // Save CV input data before overwriting with output to prevent feedback.
        juce::HeapBlock<float> cvPitchSaved(numSamples);
        juce::HeapBlock<float> cvWaveformSaved(numSamples);
        if (buffer.getNumChannels() > 0)
            juce::FloatVectorOperations::copy(cvPitchSaved, buffer.getReadPointer(0), numSamples);
        else
            juce::FloatVectorOperations::clear(cvPitchSaved, numSamples);
        if (buffer.getNumChannels() > 1)
            juce::FloatVectorOperations::copy(cvWaveformSaved, buffer.getReadPointer(1), numSamples);
        else
            juce::FloatVectorOperations::clear(cvWaveformSaved, numSamples);

        const float* cvPitchCh = cvPitchSaved;
        const float* cvWaveformCh = (buffer.getNumChannels() > 1) ? cvWaveformSaved.get() : nullptr;
        const float* cvOctaveCh = (buffer.getNumChannels() > 2) ? buffer.getReadPointer(2) : nullptr;
        const float* cvCoarseCh = (buffer.getNumChannels() > 3) ? buffer.getReadPointer(3) : nullptr;
        const float* cvFineCh = (buffer.getNumChannels() > 4) ? buffer.getReadPointer(4) : nullptr;
        const float* cvLevelCh = (buffer.getNumChannels() > 5) ? buffer.getReadPointer(5) : nullptr;

        buffer.clear();
        auto* ch0 = buffer.getWritePointer(0);

#ifndef NDEBUG
        // Log pitch debug info sparingly: first 5 calls, then every ~10 seconds
        bool shouldLog = (debugLogCounter < 5) || (debugLogCounter % 5000 == 0);
        if (shouldLog && numSamples > 0) {
            float cvSample0 = cvPitchCh ? cvPitchCh[0] : -999.0f;
            float cvOctSample0 = cvOctaveCh ? cvOctaveCh[0] : -999.0f;
            juce::Logger::writeToLog(
                "OSC [" + getName() + "] #" + juce::String(debugLogCounter) +
                " freq=" + juce::String(smoothedFreq.getTargetValue(), 1) + " midi=" + juce::String(lastMidiNote) +
                " oct=" + juce::String(octaveParam->get()) + " coarse=" + juce::String(coarseParam->get()) +
                " fine=" + juce::String(fineParam->get(), 1) + " cv0=" + juce::String(cvSample0, 4) +
                " cvOct=" + juce::String(cvOctSample0, 4));
        }
        ++debugLogCounter;
#endif

        for (int i = 0; i < numSamples; ++i) {
            float baseFreq = smoothedFreq.getNextValue();

            float cvPitch = cvPitchCh ? cvPitchCh[i] : 0.0f;
            float freq = baseFreq;

            float extraPitchShift = 0.0f;

            if (cvOctaveCh) {
                int octShift = static_cast<int>(std::round(cvOctaveCh[i] * 4.0f));
                extraPitchShift += octShift * 12.0f;
            }
            if (cvCoarseCh) {
                int coarseShift = static_cast<int>(std::round(cvCoarseCh[i] * 12.0f));
                extraPitchShift += coarseShift;
            }
            if (cvFineCh) {
                float fineShift = cvFineCh[i] * 100.0f;
                extraPitchShift += fineShift / 100.0f;
            }

            if (extraPitchShift != 0.0f) {
                freq = freq * std::pow(2.0f, extraPitchShift / 12.0f);
            }

            if (cvPitch != 0.0f) {
                float clampedCV = juce::jlimit(-5.0f, 5.0f, cvPitch);
                float totalMod = clampedCV * 2.0f; // up to 2 octaves shift
                freq = freq * std::exp2(totalMod);
            }
            freq = juce::jlimit(20.0f, 20000.0f, freq);

            int waveform = baseWaveform;
            if (cvWaveformCh) {
                int waveShift = static_cast<int>(std::round(cvWaveformCh[i] * 3.0f));
                waveform = juce::jlimit(0, 3, waveform + waveShift);
            }

            float dt = static_cast<float>(freq / currentSampleRate);
            float sample;

            if (waveform != previousWaveform) {
                fadingFromWaveform = previousWaveform;
                crossfadeSamplesRemaining = CROSSFADE_SAMPLES;
                previousWaveform = waveform;
            }

            if (crossfadeSamplesRemaining > 0) {
                float alpha = static_cast<float>(crossfadeSamplesRemaining) / CROSSFADE_SAMPLES;
                float oldSample = generateSample(fadingFromWaveform, phase, dt);
                float newSample = generateSample(waveform, phase, dt);
                sample = oldSample * alpha + newSample * (1.0f - alpha);
                --crossfadeSamplesRemaining;
            } else {
                sample = generateSample(waveform, phase, dt);
            }

            float level = levelParam->get();
            if (cvLevelCh)
                level = juce::jlimit(0.0f, 1.0f, level + cvLevelCh[i]);
            ch0[i] = sample * level;

            phase += dt;
            if (phase >= 1.0f)
                phase -= 1.0f;
        }

        // Push to visual buffer
        if (auto* vb = getVisualBuffer()) {
            for (int i = 0; i < numSamples; ++i) {
                vb->pushSample(ch0[i]);
            }
        }
    }

    float getTargetFrequency() const {
        float totalPitch =
            lastMidiNote + (octaveParam->get() * 12.0f) + (float)coarseParam->get() + (fineParam->get() / 100.0f);
        return 440.0f * std::pow(2.0f, (totalPitch - 69.0f) / 12.0f);
    }

    std::vector<ModulationTarget> getModulationTargets() const override {
        return {{"Pitch", 0}, {"Waveform", 1}, {"Octave", 2}, {"Coarse", 3}, {"Fine", 4}, {"Level", 5}};
    }
    juce::String getInputPortLabel(int i) const override {
        const juce::String labels[] = {"Pitch", "Waveform", "Octave", "Coarse", "Fine", "Level"};
        return (i >= 0 && i < 6) ? labels[i] : ModuleBase::getInputPortLabel(i);
    }
    juce::String getOutputPortLabel(int) const override { return "Audio"; }
    ModulationCategory getModulationCategory() const override { return ModulationCategory::Oscillator; }
    ModuleType getModuleType() const override { return ModuleType::Oscillator; }

private:
    float generateSample(int waveform, float phase, float dt) const {
        switch (waveform) {
        case 0:
            return generateSine(phase);
        case 1:
            return generateSquare(phase, dt);
        case 2:
            return generateSaw(phase, dt);
        case 3:
            return generateTriangle(phase, dt);
        default:
            return 0.0f;
        }
    }

    static float generateSine(float phase) { return std::sin(phase * juce::MathConstants<float>::twoPi); }

    static float generateSquare(float phase, float dt) {
        float sample = phase < 0.5f ? 1.0f : -1.0f;
        sample += polyBlep(phase, dt);
        sample -= polyBlep(std::fmod(phase + 0.5f, 1.0f), dt);
        return sample;
    }

    static float generateSaw(float phase, float dt) {
        float sample = 2.0f * phase - 1.0f;
        sample -= polyBlep(phase, dt);
        return sample;
    }

    static float generateTriangle(float phase, float dt) {
        float sample = 4.0f * std::abs(phase - 0.5f) - 1.0f;
        // PolyBLAMP: integrated polyBLEP applied at discontinuities (phase=0 and phase=0.5)
        sample += polyBlamp(phase, dt);
        sample -= polyBlamp(std::fmod(phase + 0.5f, 1.0f), dt);
        return sample;
    }

    static float polyBlamp(float t, float dt) {
        if (t < dt) {
            float n = t / dt;
            // Third-order polynomial: integrated polyBLEP
            return -dt * (n * n * n / 3.0f - n * n / 2.0f + 1.0f / 6.0f) * 4.0f;
        }
        if (t > 1.0f - dt) {
            float n = (t - 1.0f) / dt;
            return dt * (n * n * n / 3.0f + n * n / 2.0f + 1.0f / 6.0f) * 4.0f;
        }
        return 0.0f;
    }

    static float polyBlep(float t, float dt) {
        if (t < dt) {
            float n = t / dt;
            return n + n - n * n - 1.0f;
        }
        if (t > 1.0f - dt) {
            float n = (t - 1.0f) / dt;
            return n * n + n + n + 1.0f;
        }
        return 0.0f;
    }

    float phase = 0.0f;
    double currentSampleRate = 44100.0;
    float lastMidiNote = 69.0f; // Default to A4 (440Hz)

    // Waveform crossfade tracking
    int previousWaveform = 0;
    int fadingFromWaveform = 0;
    int crossfadeSamplesRemaining = 0;
    static constexpr int CROSSFADE_SAMPLES = 64;

    juce::SmoothedValue<float> smoothedFreq;
    int debugLogCounter = 0;
    juce::AudioParameterChoice* waveformParam;
    juce::AudioParameterInt* octaveParam;
    juce::AudioParameterInt* coarseParam;
    juce::AudioParameterFloat* fineParam;
    juce::AudioParameterFloat* levelParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorModule)
};
