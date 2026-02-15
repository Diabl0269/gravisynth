#include "Modules/SequencerModule.h"
#include <gtest/gtest.h>

class SequencerModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        seq = std::make_unique<SequencerModule>();
        seq->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<SequencerModule> seq;
};

TEST_F(SequencerModuleTest, StoppedByDefault) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    seq->processBlock(buffer, midi);
    EXPECT_TRUE(midi.isEmpty());
}

TEST_F(SequencerModuleTest, GeneratesMidiWhenRunning) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[0]);
    runParam->setValueNotifyingHost(1.0f);

    // Initial state: samplesUntilNextBeat = 0
    // First processBlock should trigger a note
    seq->processBlock(buffer, midi);

    EXPECT_FALSE(midi.isEmpty());

    bool foundNoteOn = false;
    for (const auto metadata : midi) {
        if (metadata.getMessage().isNoteOn()) {
            foundNoteOn = true;
            break;
        }
    }
    EXPECT_TRUE(foundNoteOn);
}

TEST_F(SequencerModuleTest, StepsAdvance) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[0]);
    runParam->setValueNotifyingHost(1.0f);

    int initialStep = seq->currentActiveStep;
    seq->processBlock(buffer, midi);

    // It should advance after the beat timer expires.
    // In the first block, it triggers step 0 and advances currentStep to 1.
    // currentActiveStep is updated TO currentStep when the timer expires.
    // Wait, let's check code:
    // samplesUntilNextBeat <= 0:
    //   currentActiveStep = currentStep (0)
    //   currentStep = (0 + 1) % 8 (1)

    EXPECT_EQ(seq->currentActiveStep, 0);

    // Need to process enough samples to reach next beat
    // 120 BPM = 0.5s = 22050 samples at 44.1k
    int samplesToNextBeat = 22050;
    while (samplesToNextBeat > 0) {
        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer m;
        seq->processBlock(b, m);
        samplesToNextBeat -= 512;
    }

    // Should have advanced
    EXPECT_EQ(seq->currentActiveStep, 1);
}

TEST_F(SequencerModuleTest, SendsFilterEnvCC) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[0]);
    runParam->setValueNotifyingHost(1.0f);

    seq->processBlock(buffer, midi);

    bool foundCC74 = false;
    for (const auto metadata : midi) {
        auto msg = metadata.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 74) {
            foundCC74 = true;
            break;
        }
    }
    EXPECT_TRUE(foundCC74);
}
