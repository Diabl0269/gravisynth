#include "Modules/PolyMidiModule.h"
#include <gtest/gtest.h>

class PolyMidiModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<PolyMidiModule>();
        module->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<PolyMidiModule> module;
};

TEST_F(PolyMidiModuleTest, AcceptsMidi) { EXPECT_TRUE(module->acceptsMidi()); }

TEST_F(PolyMidiModuleTest, NoteOnAllocatesVoice) {
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

    juce::AudioBuffer<float> buffer(16, 512);
    module->processBlock(buffer, midi);

    // After Note On, at least one voice should be active
    uint8_t mask = module->getActiveVoiceMask();
    EXPECT_NE(mask, 0);
}

TEST_F(PolyMidiModuleTest, NoteOffDeactivatesVoice) {
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);

    juce::AudioBuffer<float> buffer(16, 512);
    module->processBlock(buffer, midi);

    EXPECT_NE(module->getActiveVoiceMask(), 0);

    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    module->processBlock(buffer, midi);

    // Note off should immediately set voice inactive (though it might still have smoothed release in audio)
    EXPECT_EQ(module->getActiveVoiceMask(), 0);
}

TEST_F(PolyMidiModuleTest, VoiceStealingLRU) {
    juce::AudioBuffer<float> buffer(16, 512);

    // Switch on 8 voices
    for (int i = 0; i < 8; ++i) {
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60 + i, 0.8f), 0);
        module->processBlock(buffer, midi);
        juce::Thread::sleep(2); // Ensure timestamps differ
    }

    EXPECT_EQ(module->getActiveVoiceMask(), 0xFF);

    // Add 9th voice - should steal the first one (note 60)
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 72, 0.8f), 0);
    module->processBlock(buffer, midi);

    // We can't easily check which one was stolen without exposing voices,
    // but we can check if it stays at 8 active voices
    EXPECT_EQ(module->getActiveVoiceMask(), 0xFF);
}

TEST_F(PolyMidiModuleTest, AllNotesOff) {
    juce::AudioBuffer<float> buffer(16, 512);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 64, 0.8f), 0);
    module->processBlock(buffer, midi);

    EXPECT_NE(module->getActiveVoiceMask(), 0);

    midi.clear();
    midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
    module->processBlock(buffer, midi);

    EXPECT_EQ(module->getActiveVoiceMask(), 0);
}

TEST_F(PolyMidiModuleTest, RenderChunk) {
    juce::AudioBuffer<float> buffer(16, 512);
    juce::MidiBuffer midi;

    // Note 60 = 261.63 Hz
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    module->processBlock(buffer, midi);

    // Voice 0 should have frequency (~261) in pitch channel (0) and 1.0 in gate (8)
    // Wait, smoothed value takes time. Let's check after some samples.
    EXPECT_GT(buffer.getSample(0, 511), 200.0f);
    EXPECT_GT(buffer.getSample(8, 511), 0.9f);
}
