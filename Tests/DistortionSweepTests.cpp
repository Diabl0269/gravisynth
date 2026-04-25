#include "Modules/FX/DistortionModule.h"
#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

class DistortionSweep : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<DistortionModule>();
        module->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<DistortionModule> module;
};

TEST_F(DistortionSweep, MixZeroSineVerification) {
    // With mix = 0, output should be identical to input
    auto* driveP = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[1]);
    auto* mixP = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[2]);
    auto* osP = dynamic_cast<juce::AudioParameterChoice*>(module->getParameters()[3]);

    // Set oversampling to Off for 0 latency transparency
    *osP = 0;
    // Set drive to max to ensure we WOULD see distortion if mix was non-zero
    driveP->setValueNotifyingHost(1.0f);
    mixP->setValueNotifyingHost(0.0f); // fully dry

    module->prepareToPlay(44100.0, 512); // reset smoothers to target values

    juce::AudioBuffer<float> buffer(4, 512);
    buffer.clear();

    // Fill with a sine wave
    for (int i = 0; i < 512; ++i) {
        float val = std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
        buffer.setSample(0, i, val);
        buffer.setSample(1, i, val);
    }

    juce::AudioBuffer<float> original = buffer;
    juce::MidiBuffer midi;

    module->processBlock(buffer, midi);

    for (int i = 0; i < 512; ++i) {
        EXPECT_NEAR(buffer.getSample(0, i), original.getSample(0, i), 1e-5f) << "Sample " << i << " mismatch at mix=0";
    }
}
