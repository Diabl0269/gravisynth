#pragma once

#include "../ModuleBase.h"
#include <juce_dsp/juce_dsp.h>

class DistortionModule : public ModuleBase {
public:
    DistortionModule()
        : ModuleBase("Distortion", 2, 2) // Stereo FX
    {
        addParameter(driveParam = new juce::AudioParameterFloat("drive", "Drive", 1.0f, 10.0f, 1.0f));
        addParameter(mixParam = new juce::AudioParameterFloat("mix", "Mix", 0.0f, 1.0f, 0.5f));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // 2x oversampling with polyphase IIR half-band filter
        oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // numChannels
            1, // oversampling order (2x)
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR);

        oversampling->initProcessing(static_cast<size_t>(samplesPerBlock));

        smoothedMix.reset(sampleRate, 0.005); // 5ms ramp
        smoothedMix.setCurrentAndTargetValue(*mixParam);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        juce::ignoreUnused(midiMessages);

        float drive = *driveParam;
        smoothedMix.setTargetValue(*mixParam);

        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        // Save dry signal
        juce::AudioBuffer<float> dryBuffer;
        dryBuffer.makeCopyOf(buffer);

        // Upsample
        juce::dsp::AudioBlock<float> inputBlock(buffer);
        auto oversampledBlock = oversampling->processSamplesUp(inputBlock);

        applyDistortion(oversampledBlock, drive);
        oversampling->processSamplesDown(inputBlock);
        blendWetDry(buffer, dryBuffer, numChannels, numSamples);
    }

    double getLatencyInSamples() const {
        return oversampling ? oversampling->getLatencyInSamples() : 0.0;
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterFloat* mixParam;
};
