#pragma once

#include "../ModuleBase.h"

class DelayModule : public ModuleBase {
public:
    DelayModule()
        : ModuleBase("Delay", 2, 2) {
        addParameter(timeParam = new juce::AudioParameterFloat("time", "Time (ms)", 1.0f, 1000.0f, 250.0f));
        addParameter(feedbackParam = new juce::AudioParameterFloat("feedback", "Feedback", 0.0f, 0.95f, 0.5f));
        addParameter(mixParam = new juce::AudioParameterFloat("mix", "Mix", 0.0f, 1.0f, 0.3f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        this->sampleRate = sampleRate;
        int maxDelaySamples = (int)(1.0f * sampleRate); // 1 second
        delayBuffer.setSize(2, maxDelaySamples + samplesPerBlock);
        delayBuffer.clear();
        writePos = 0;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        float delayTimeMs = *timeParam;
        float feedback = *feedbackParam;
        float mix = *mixParam;

        int delaySamples = (int)(delayTimeMs * 0.001f * sampleRate);
        int bufferSize = buffer.getNumSamples();
        int delayBufferSize = delayBuffer.getNumSamples();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            auto* delayData = delayBuffer.getWritePointer(ch % delayBuffer.getNumChannels());

            int localWritePos = writePos;

            for (int i = 0; i < bufferSize; ++i) {
                float input = data[i];

                int readPos = (localWritePos - delaySamples + delayBufferSize) % delayBufferSize;
                float delayedSample = delayData[readPos];

                // Update delay buffer with feedback
                delayData[localWritePos] = input + (delayedSample * feedback);

                localWritePos = (localWritePos + 1) % delayBufferSize;

                // Output mix
                data[i] = (delayedSample * mix) + (input * (1.0f - mix));
            }

            if (ch == buffer.getNumChannels() - 1)
                writePos = localWritePos;
        }
    }

private:
    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;
    double sampleRate = 44100.0;

    juce::AudioParameterFloat* timeParam;
    juce::AudioParameterFloat* feedbackParam;
    juce::AudioParameterFloat* mixParam;
};
