#pragma once

#include "ModuleBase.h"
#include <cmath>

class OscillatorModule : public ModuleBase {
public:
    OscillatorModule()
        : ModuleBase("Oscillator", 1, 1) // 1 input (freq/pitch mod), 1 output
    {
        addParameter(waveformParam = new juce::AudioParameterChoice("waveform", "Waveform",
                                                                    {"Sine", "Square", "Saw", "Triangle"}, 0));
        addParameter(frequencyParam = new juce::AudioParameterFloat("frequency", "Frequency", 20.0f, 20000.0f, 440.0f));

        enableVisualBuffer(true);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(samplesPerBlock);
        currentSampleRate = sampleRate;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        // Process MIDI for Pitch
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            if (msg.isNoteOn()) {
                float frequency = juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
                *frequencyParam = frequency;
                phase = 0.0f; // Reset phase on note-on
            }
        }

        if (buffer.getNumChannels() == 0)
            return;

        float freq = *frequencyParam;
        float dt = static_cast<float>(freq / currentSampleRate); // phase increment per sample
        int waveform = waveformParam->getIndex();
        int numSamples = buffer.getNumSamples();
        auto* ch0 = buffer.getWritePointer(0);

        for (int i = 0; i < numSamples; ++i) {
            float sample = generateSample(waveform, phase, dt);
            ch0[i] = sample;

            phase += dt;
            if (phase >= 1.0f)
                phase -= 1.0f;
        }

        // Copy channel 0 to other channels
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);

        // Push to visual buffer
        if (auto* vb = getVisualBuffer()) {
            for (int i = 0; i < numSamples; ++i) {
                vb->pushSample(ch0[i]);
            }
        }
    }

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
            return generateTriangle(phase);
        default:
            return 0.0f;
        }
    }

    static float generateSine(float phase) {
        return std::sin(phase * juce::MathConstants<float>::twoPi);
    }

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

    static float generateTriangle(float phase) {
        return 4.0f * std::abs(phase - 0.5f) - 1.0f;
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

    juce::AudioParameterChoice* waveformParam;
    juce::AudioParameterFloat* frequencyParam;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscillatorModule)
};
