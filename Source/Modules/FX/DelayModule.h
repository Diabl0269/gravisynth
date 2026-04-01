#pragma once

#include "../ModuleBase.h"

class DelayModule : public ModuleBase {
public:
    DelayModule()
        : ModuleBase("Delay", 5, 2) { // 2 Audio + 3 CV (Time, Feedback, Mix)
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

        smoothedTime.reset(sampleRate, 0.05);      // 50ms ramp for time
        smoothedFeedback.reset(sampleRate, 0.005); // 5ms ramp
        smoothedMix.reset(sampleRate, 0.005);      // 5ms ramp

        smoothedTime.setCurrentAndTargetValue(*timeParam);
        smoothedFeedback.setCurrentAndTargetValue(*feedbackParam);
        smoothedMix.setCurrentAndTargetValue(*mixParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        smoothedTime.setTargetValue(*timeParam);
        smoothedFeedback.setTargetValue(*feedbackParam);
        smoothedMix.setTargetValue(*mixParam);

        int bufferSize = buffer.getNumSamples();
        int delayBufferSize = delayBuffer.getNumSamples();

        const float* cvTime = (buffer.getNumChannels() > 2) ? buffer.getReadPointer(2) : nullptr;
        const float* cvFeedback = (buffer.getNumChannels() > 3) ? buffer.getReadPointer(3) : nullptr;
        const float* cvMix = (buffer.getNumChannels() > 4) ? buffer.getReadPointer(4) : nullptr;

        for (int ch = 0; ch < 2; ++ch) { // Process stereo audio (first 2 channels)
            if (ch >= buffer.getNumChannels())
                break;

            auto* data = buffer.getWritePointer(ch);
            auto* delayData = delayBuffer.getWritePointer(ch);

            int localWritePos = writePos;

            for (int i = 0; i < bufferSize; ++i) {
                float timeMod = cvTime ? cvTime[i] : 0.0f;
                float feedMod = cvFeedback ? cvFeedback[i] : 0.0f;
                float mixMod = cvMix ? cvMix[i] : 0.0f;

                float currentTime = juce::jlimit(1.0f, 1000.0f, smoothedTime.getNextValue() + (timeMod * 500.0f));
                float currentFeedback = juce::jlimit(0.0f, 0.95f, smoothedFeedback.getNextValue() + feedMod);
                float currentMix = juce::jlimit(0.0f, 1.0f, smoothedMix.getNextValue() + mixMod);

                float input = data[i];

                float delaySamplesF = currentTime * 0.001f * static_cast<float>(sampleRate);
                float readPosF =
                    static_cast<float>(localWritePos) - delaySamplesF + static_cast<float>(delayBufferSize);
                float delayedSample = linearInterpolate(delayData, delayBufferSize, readPosF);

                delayData[localWritePos] = input + (delayedSample * currentFeedback);
                localWritePos = (localWritePos + 1) % delayBufferSize;

                data[i] = (delayedSample * currentMix) + (input * (1.0f - currentMix));
            }

            if (ch == 1 || buffer.getNumChannels() == 1)
                writePos = localWritePos;
        }

        // Output logic for non-audio channels (pass through CV if needed, but usually not)
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, bufferSize);
        }
    }

    std::vector<ModulationTarget> getModulationTargets() const override {
        return {{"Time", 2}, {"Feedback", 3}, {"Mix", 4}};
    }

    ModulationCategory getModulationCategory() const override { return ModulationCategory::FX; }

private:
    static float linearInterpolate(const float* buffer, int bufferSize, float fractionalPos) {
        while (fractionalPos < 0.0f)
            fractionalPos += static_cast<float>(bufferSize);

        int pos0 = static_cast<int>(fractionalPos) % bufferSize;
        int pos1 = (pos0 + 1) % bufferSize;
        float frac = fractionalPos - std::floor(fractionalPos);

        return buffer[pos0] * (1.0f - frac) + buffer[pos1] * frac;
    }

    juce::AudioBuffer<float> delayBuffer;
    int writePos = 0;
    double sampleRate = 44100.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedTime;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFeedback;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

    juce::AudioParameterFloat* timeParam;
    juce::AudioParameterFloat* feedbackParam;
    juce::AudioParameterFloat* mixParam;
};
