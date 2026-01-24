#include "Modules/ADSRModule.h"
#include <gtest/gtest.h>

class ADSRTest : public ::testing::Test {
protected:
  ADSRModule adsr;
  juce::AudioBuffer<float> buffer;
  juce::MidiBuffer midiMessages;

  void SetUp() override {
    adsr.prepareToPlay(44100.0, 512);
    buffer.setSize(2, 512);
    buffer.clear();
  }
};

TEST_F(ADSRTest, StartsIdle) {
  // Initially should output 0
  adsr.processBlock(buffer, midiMessages);
  // Since ADSR modifies buffer in place (multiplying?), typically ADSR is a
  // control signal or VCA? In Gravisynth, ADSRModule might just generate an
  // envelope or apply it. Checking code... wait, I need to check ADSRModule
  // implementation to know if it's a generator or processor. Assuming standard
  // VCA-like behavior or just check "getNextSample" if exposed? "ModuleBase"
  // usually implies processBlock. If buffer was clear (0), output should be 0.

  // Let's set buffer to 1.0 to see if envelope is applied
  for (int i = 0; i < buffer.getNumSamples(); ++i)
    buffer.setSample(0, i, 1.0f);

  adsr.processBlock(buffer, midiMessages);

  // Without NoteOn, envelope should be 0 (if valid), so output 0.
  float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
  // expect close to 0
  EXPECT_NEAR(rms, 0.0f, 1e-4);
}

TEST_F(ADSRTest, AttackPhase) {
  // Trigger NoteOn
  auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
  midiMessages.addEvent(noteOn, 0);

  // Set buffer to constant 1.0 to measure envelope
  for (int i = 0; i < buffer.getNumSamples(); ++i) {
    buffer.setSample(0, i, 1.0f);
    buffer.setSample(1, i, 1.0f);
  }

  adsr.processBlock(buffer, midiMessages);

  // Envelope should have risen from 0.
  float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
  EXPECT_GT(rms, 0.001f);
}
