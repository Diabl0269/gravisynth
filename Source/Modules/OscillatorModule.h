#pragma once

#include "ModuleBase.h"
#include <cmath>

class OscillatorModule : public ModuleBase {
public:
    OscillatorModule()
        : ModuleBase("Oscillator", 5, 1) // 1 pitch mod, 4 param mod inputs, 1 output
    {
        addParameter(waveformParam = new juce::AudioParameterChoice("waveform", "Waveform",
                                                                    {"Sine", "Square", "Saw", "Triangle"}, 0));
        addParameter(octaveParam = new juce::AudioParameterInt("octave", "Octave", -4, 4, 0));
        addParameter(coarseParam = new juce::AudioParameterInt("coarse", "Coarse", -12, 12, 0));
        addParameter(fineParam = new juce::AudioParameterFloat("fine", "Fine", -100.0f, 100.0f, 0.0f));

        enableVisualBuffer(true);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        currentSampleRate = sampleRate;
        smoothedFreq.reset(sampleRate, 0.005); // 5ms glide
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        // Process MIDI for Pitch
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn()) {
                lastMidiNote = (float)msg.getNoteNumber();
            }
        }

        if (buffer.getNumChannels() == 0)
            return;

        float totalPitch =
            lastMidiNote + (octaveParam->get() * 12.0f) + (float)coarseParam->get() + (fineParam->get() / 100.0f);
        float targetFreq = 440.0f * std::pow(2.0f, (totalPitch - 69.0f) / 12.0f);
        smoothedFreq.setTargetValue(targetFreq);

        int baseWaveform = waveformParam->getIndex();
        int numSamples = buffer.getNumSamples();

        const float* cvPitchCh = (buffer.getNumChannels() > 0) ? buffer.getReadPointer(0) : nullptr;
        const float* cvWaveformCh = (buffer.getNumChannels() > 1) ? buffer.getReadPointer(1) : nullptr;
        const float* cvOctaveCh = (buffer.getNumChannels() > 2) ? buffer.getReadPointer(2) : nullptr;
        const float* cvCoarseCh = (buffer.getNumChannels() > 3) ? buffer.getReadPointer(3) : nullptr;
        const float* cvFineCh = (buffer.getNumChannels() > 4) ? buffer.getReadPointer(4) : nullptr;

        auto* ch0 = buffer.getWritePointer(0);

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
                float totalMod = cvPitch * 2.0f; // up to 2 octaves shift
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

            ch0[i] = sample;

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
        return {{"Pitch", 0}, {"Waveform", 1}, {"Octave", 2}, {"Coarse", 3}, {"Fine", 4}};
    }
    ModulationCategory getModulationCategory() const override { return ModulationCategory::Oscillator; }

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
    juce::AudioParameterChoice* waveformParam;
    juce::AudioParameterInt* octaveParam;
    juce::AudioParameterInt* coarseParam;
    juce::AudioParameterFloat* fineParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorModule)
};
