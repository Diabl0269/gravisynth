#include "Modules/OscillatorModule.h"
#include <gtest/gtest.h>

class OscillatorTest : public ::testing::Test {
protected:
  OscillatorModule oscillator;
  juce::AudioBuffer<float> buffer;
  juce::MidiBuffer midiMessages;

  void SetUp() override {
    oscillator.prepareToPlay(44100.0, 512);
    buffer.setSize(2, 512);
    buffer.clear();
  }
};

TEST_F(OscillatorTest, ProducesSilenceWhenNoChannels) {
  juce::AudioBuffer<float> emptyBuffer;
  oscillator.processBlock(emptyBuffer, midiMessages);
  // Should not crash
}

TEST_F(OscillatorTest, GeneratesSignal) {
  // Oscillator is running by default (freq 440)
  oscillator.processBlock(buffer, midiMessages);

  float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
  EXPECT_GT(rms, 0.0f);
}

TEST_F(OscillatorTest, RespondsToMidiMethod) {
  // Note On 69 (A4 = 440Hz)
  auto noteOn = juce::MidiMessage::noteOn(1, 69, (juce::uint8)100);
  midiMessages.addEvent(noteOn, 0);

  oscillator.processBlock(buffer, midiMessages);

  // Check parameters (we can access them via getParameters)
  // But ideally we'd check the frequency param directly if exposed,
  // or trust the audio output (harder to test freq in unit test without FFT)
  // For now, just ensure it processed without crashing and output signal.

  // Changing note
  midiMessages.clear();
  auto noteOn2 = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100); // C4
  midiMessages.addEvent(noteOn2, 0);
  oscillator.processBlock(buffer, midiMessages);

  float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
  EXPECT_GT(rms, 0.0f);
}

TEST_F(OscillatorTest, ChangesWaveform) {
  const auto &params = oscillator.getParameters();
  juce::AudioProcessorParameter *waveParam = nullptr;
  for (auto *p : params) {
    if (p->getName(100) == "Waveform") {
      waveParam = p;
      break;
    }
  }
  ASSERT_NE(waveParam, nullptr);

  // Test all 4 waveforms (0=Sine, 1=Square, 2=Saw, 3=Triangle)
  // Logic: Set param -> process -> check signal > 0
  // Since we rely on updateWaveform() being called inside processBlock(),
  // calling processBlock is essential.

  float values[] = {0.0f, 0.33f, 0.66f,
                    1.0f}; // Map to index 0, 1, 2, 3 approximately

  for (float v : values) {
    waveParam->setValueNotifyingHost(v);
    buffer.clear();
    oscillator.processBlock(buffer, midiMessages);
    float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    EXPECT_GT(rms, 0.0f) << "Failed for waveform value " << v;
  }
}
