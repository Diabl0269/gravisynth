#pragma once

#include "ModuleBase.h"

class OscillatorModule : public ModuleBase {
public:
  OscillatorModule()
      : ModuleBase("Oscillator", 1, 1) // 1 input (freq/pitch mod), 1 output
  {
    // Default params
    addParameter(
        waveformParam = new juce::AudioParameterChoice(
            "waveform", "Waveform", {"Sine", "Square", "Saw", "Triangle"}, 0));
    addParameter(frequencyParam = new juce::AudioParameterFloat(
                     "frequency", "Frequency", 20.0f, 20000.0f, 440.0f));

    enableVisualBuffer(true);
  }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::dsp::ProcessSpec spec = {
        sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2};
    oscillator.prepare(spec);
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {

    // Process MIDI for Pitch
    for (const auto metadata : midiMessages) {
      auto msg = metadata.getMessage();
      if (msg.isNoteOn()) {
        float frequency =
            juce::MidiMessage::getMidiNoteInHertz(msg.getNoteNumber());
        // Update param so UI reflects it? Or just internal?
        // Parameter is float, let's just use internal or set param.
        // Setting param notifies listeners.
        *frequencyParam = frequency;
        oscillator.reset(); // Reset phase to 0 to avoid clicks
      }
    }

    // Update params
    updateWaveform();
    oscillator.setFrequency(*frequencyParam);

    // Safe channel access
    if (buffer.getNumChannels() == 0)
      return;

    // Render
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    oscillator.process(context);

    // Push to visual buffer
    if (auto *vb = getVisualBuffer()) {
      auto *ch = buffer.getReadPointer(0);
      for (int i = 0; i < buffer.getNumSamples(); ++i) {
        vb->pushSample(ch[i]);
      }
    }
  }

private:
  void updateWaveform() {
    switch (waveformParam->getIndex()) {
    case 0:
      oscillator.initialise([](float x) { return std::sin(x); });
      break;
    case 1:
      oscillator.initialise([](float x) { return x < 0.0f ? -1.0f : 1.0f; });
      break;
    case 2:
      // Naive saw: x is 0..2pi (usually) or -pi..pi in JUCE dsp::Oscillator?
      // Actually dsp::Oscillator implementation guarantees 0..2pi or something
      // specific. Wait, juce::dsp::Oscillator uses a phase incrementer. The
      // argument x to the lambda is the phase (0 to 2*PI). Let's verify
      // standard behaviour. Usually 0 to 2PI. If 0..2PI: x / PI -> 0..2. Minus
      // 1 -> -1..1.
      oscillator.initialise(
          [](float x) { return (x / juce::MathConstants<float>::pi) - 1.0f; });
      break;
    case 3:
      oscillator.initialise([](float x) {
        return 2.0f * std::abs(x / juce::MathConstants<float>::pi) - 1.0f;
      });
      break; // Naive triangle
      // Better to use juce::dsp::Oscillator lookup tables for anti-aliasing but
      // this is simple start
    }
  }

  juce::dsp::Oscillator<float> oscillator;
  juce::AudioParameterChoice *waveformParam;
  juce::AudioParameterFloat *frequencyParam;
};
