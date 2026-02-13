#include "Modules/LFOModule.h"
#include <gtest/gtest.h>

class LFOModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        lfo = std::make_unique<LFOModule>();
        lfo->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<LFOModule> lfo;
};

TEST_F(LFOModuleTest, WaveformOutput) {
    juce::AudioBuffer<float> buffer(2, 44100); // 1 second
    juce::MidiBuffer midi;

    // Test Sine
    auto* shapeParam = dynamic_cast<juce::AudioParameterChoice*>(lfo->getParameters()[0]);
    shapeParam->setValueNotifyingHost(0.0f); // Sine

    lfo->processBlock(buffer, midi);

    // Check for variation in output
    float minVal = 1.0f;
    float maxVal = -1.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        float s = buffer.getSample(0, i);
        minVal = std::min(minVal, s);
        maxVal = std::max(maxVal, s);
    }
    EXPECT_LT(minVal, -0.9f);
    EXPECT_GT(maxVal, 0.9f);
}

TEST_F(LFOModuleTest, BipolarUnipolar) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    auto* bipolarParam = dynamic_cast<juce::AudioParameterBool*>(lfo->getParameters()[2]);

    // Unipolar
    bipolarParam->setValueNotifyingHost(0.0f);
    lfo->processBlock(buffer, midi);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        EXPECT_GE(buffer.getSample(0, i), 0.0f);
    }

    // Bipolar
    bipolarParam->setValueNotifyingHost(1.0f);
    juce::AudioBuffer<float> buffer2(2, 44100);
    lfo->processBlock(buffer2, midi);
    bool foundNegative = false;
    for (int i = 0; i < buffer2.getNumSamples(); ++i) {
        if (buffer2.getSample(0, i) < 0.0f) {
            foundNegative = true;
            break;
        }
    }
    EXPECT_TRUE(foundNegative);
}

TEST_F(LFOModuleTest, Retrigger) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    auto* retrigParam = dynamic_cast<juce::AudioParameterBool*>(lfo->getParameters()[5]);
    retrigParam->setValueNotifyingHost(1.0f);

    // Process a bit to advance phase
    lfo->processBlock(buffer, midi);

    // Send Note On
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.5f), 0);
    lfo->processBlock(buffer, midi);

    // Phase should have reset. For Sine (default), phase 0 means 0.0
    EXPECT_NEAR(buffer.getSample(0, 0), 0.0f, 0.01f);
}

TEST_F(LFOModuleTest, SampleAndHold) {
    juce::AudioBuffer<float> buffer(2, 44100);
    juce::MidiBuffer midi;

    auto* shapeParam = dynamic_cast<juce::AudioParameterChoice*>(lfo->getParameters()[0]);
    shapeParam->setValueNotifyingHost(1.0f); // S&H is index 4, but setValue is normalized.
    // Wait, let's use getParameters() index.
    // LFO params: shape, mode, bipolar, rateHz, rateSync, retrig, level, glide

    auto params = lfo->getParameters();
    auto* shape = dynamic_cast<juce::AudioParameterChoice*>(params[0]);
    *shape = 4; // S&H

    lfo->processBlock(buffer, midi);

    // S&H should stay constant for a while depending on rate
    EXPECT_EQ(buffer.getSample(0, 0), buffer.getSample(0, 1));
}
