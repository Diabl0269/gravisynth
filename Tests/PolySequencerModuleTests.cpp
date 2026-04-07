#include "Modules/PolySequencerModule.h"
#include <gtest/gtest.h>

class PolySequencerModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        seq = std::make_unique<PolySequencerModule>();
        seq->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<PolySequencerModule> seq;
};

TEST_F(PolySequencerModuleTest, ProducesMidi) { EXPECT_TRUE(seq->producesMidi()); }

TEST_F(PolySequencerModuleTest, StoppedByDefault) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    seq->processBlock(buffer, midi);
    EXPECT_TRUE(midi.isEmpty());
}

TEST_F(PolySequencerModuleTest, GeneratesChordWhenRunning) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    // Run
    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[1]);
    runParam->setValueNotifyingHost(1.0f);

    // Set Minor chord for step 0
    auto* chord0 = dynamic_cast<juce::AudioParameterChoice*>(seq->getParameters()[12 + 0]); // Step 1 Chord
    *chord0 = 2;                                                                            // Minor

    seq->processBlock(buffer, midi);

    // Minor chord should have 3 notes (root, root+3, root+7)
    int noteCount = 0;
    for (const auto meta : midi) {
        if (meta.getMessage().isNoteOn())
            noteCount++;
    }
    EXPECT_EQ(noteCount, 3);
}

TEST_F(PolySequencerModuleTest, StepsAdvance) {
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;

    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[1]);
    runParam->setValueNotifyingHost(1.0f);

    int initialStep = seq->currentActiveStep;
    seq->processBlock(buffer, midi);

    // Beat 120BPM = 0.5s = 22050 samples
    int samples = 22050 + 512;
    while (samples > 0) {
        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer m;
        seq->processBlock(b, m);
        samples -= 512;
    }

    EXPECT_EQ(seq->currentActiveStep, 1);
}

TEST_F(PolySequencerModuleTest, BPMChange) {
    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[1]);
    auto* bpmParam = dynamic_cast<juce::AudioParameterFloat*>(seq->getParameters()[2]);
    runParam->setValueNotifyingHost(1.0f);

    // Fast BPM
    bpmParam->setValueNotifyingHost(bpmParam->getNormalisableRange().convertTo0to1(300.0f));

    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    seq->processBlock(buffer, midi);

    // 300 BPM = 5 beats per sec = 0.2s per beat = 8820 samples
    int samples = 9000;
    while (samples > 0) {
        juce::AudioBuffer<float> b(2, 512);
        juce::MidiBuffer m;
        seq->processBlock(b, m);
        samples -= 512;
    }

    EXPECT_EQ(seq->currentActiveStep, 1);
}

TEST_F(PolySequencerModuleTest, AllChordTypes) {
    auto* runParam = dynamic_cast<juce::AudioParameterBool*>(seq->getParameters()[1]);
    runParam->setValueNotifyingHost(1.0f);

    for (int i = 0; i < 8; ++i) {
        auto* chord0 = dynamic_cast<juce::AudioParameterChoice*>(seq->getParameters()[12]);
        *chord0 = i;

        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        seq->processBlock(buffer, midi);

        // Just verify it doesn't crash and generates at least 1 note
        int noteCount = 0;
        for (const auto meta : midi)
            if (meta.getMessage().isNoteOn())
                noteCount++;
        EXPECT_GE(noteCount, 1);

        // Prepare for next beat trigger
        seq->prepareToPlay(44100.0, 512);
    }
}
