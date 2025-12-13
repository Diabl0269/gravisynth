#pragma once

#include "ModuleBase.h"
#include <JuceHeader.h> // Ensure we have attributes

class SequencerModule : public ModuleBase {
public:
  SequencerModule()
      : ModuleBase("Sequencer", 0, 0) // Generates MIDI
  {
    addParameter(runParam = new juce::AudioParameterBool("run", "Run", false));
    addParameter(bpmParam = new juce::AudioParameterFloat("bpm", "BPM", 30.0f,
                                                          300.0f, 120.0f));

    // Gate Lengths
    for (int i = 0; i < 8; ++i) {
      juce::String name = "Gate " + juce::String(i + 1);
      addParameter(gateParams[i] = new juce::AudioParameterFloat(
                       name, name, 0.1f, 1.0f, 0.5f));
    }

    // Pitches
    // Pitches (F Phrygian Dominant: F, Gb, A, Bb, C, Db, Eb)
    // Notes around C3 (48) - C4 (60)
    int scale[] = {53, 54, 57, 58, 60, 61, 63, 65, 66, 69};
    int numNotes = 10;

    // Seed random (basic) - actually just deterministic pseudo-random for now
    // or manually pick interesting defaults.
    int defaults[] = {53, 65, 54, 61,
                      53, 57, 54, 60}; // F3, F4, Gb3, Db4, F3, A3, Gb3, C4

    for (int i = 0; i < 8; ++i) {
      juce::String name = "Pitch " + juce::String(i + 1);

      auto stringFromInt = [](int value, int) {
        return juce::MidiMessage::getMidiNoteName(value, true, true, 3);
      };

      auto valueFromText = [](const juce::String &text) {
        for (int n = 0; n < 128; ++n) {
          if (juce::MidiMessage::getMidiNoteName(n, true, true, 3)
                  .equalsIgnoreCase(text))
            return n;
        }
        return text.getIntValue();
      };

      addParameter(stepParams[i] = new juce::AudioParameterInt(
                       name, name, 0, 127, defaults[i], "", stringFromInt,
                       valueFromText));
    }

    // Filter Envelope Amounts
    for (int i = 0; i < 8; ++i) {
      juce::String name = "F.Env " + juce::String(i + 1);
      addParameter(filterEnvParams[i] = new juce::AudioParameterFloat(
                       name, name, 0.0f, 1.0f, 0.5f));
    }
  }

  // Exposed for UI
  std::atomic<int> currentActiveStep{0};

  void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    localSampleRate = sampleRate;
    juce::ignoreUnused(samplesPerBlock);
    currentStep = 0;
    currentActiveStep = 0;
    samplesUntilNextBeat = 0;
    samplesUntilNoteOff = 0;
    lastNote = -1;
  }

  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override {
    juce::ignoreUnused(buffer);

    if (!*runParam) {
      return;
    }

    auto samplesPerBeat = (60.0 / *bpmParam) * localSampleRate;
    auto numSamples = buffer.getNumSamples();

    samplesUntilNextBeat -= numSamples;

    // Note On Logic
    if (samplesUntilNextBeat <= 0) {

      // Update UI atomics always
      currentActiveStep = currentStep;

      // Read params for the NEW step
      int noteVal = *stepParams[currentStep];
      float gateLen = *gateParams[currentStep];
      float filterAmt = *filterEnvParams[currentStep];

      int noteDuration = (int)(samplesPerBeat * gateLen);

      // Handle Note Off for the PREVIOUS note if it's still playing
      if (lastNote > 0) {
        auto noteOff = juce::MidiMessage::noteOff(1, lastNote);
        midiMessages.addEvent(noteOff, 0);
        lastNote = -1; // Flag as off
      }

      samplesUntilNoteOff = 0; // Reset timer

      if (noteVal > 0) {
        // Send CC for Filter Env Amount (CC 74)
        auto cc = juce::MidiMessage::controllerEvent(1, 74,
                                                     (int)(filterAmt * 127.0f));
        midiMessages.addEvent(cc, 0);

        // Note On
        auto note = juce::MidiMessage::noteOn(1, noteVal, (juce::uint8)100);
        midiMessages.addEvent(note, 0);

        // Update state
        lastNote = noteVal;
        samplesUntilNoteOff = noteDuration;
      } else {
        // Rest logic
        if (lastNote > 0) {
          auto noteOff = juce::MidiMessage::noteOff(1, lastNote);
          midiMessages.addEvent(noteOff, 0);
        }
        lastNote = -1;
        samplesUntilNoteOff = 0;
      }

      // Advance
      currentStep = (currentStep + 1) % 8;

      // Reset beat timer
      samplesUntilNextBeat += (int)samplesPerBeat;
    }

    // Note Off Logic (Global for the active note)
    // Only decrement if we actually have a note playing
    if (lastNote > 0 && samplesUntilNoteOff > 0) {
      samplesUntilNoteOff -= numSamples;
      if (samplesUntilNoteOff <= 0) {
        auto noteOff = juce::MidiMessage::noteOff(1, lastNote);
        midiMessages.addEvent(noteOff, 0); // Immediate
        lastNote = -1;
      }
    }
  }

private:
  double localSampleRate = 44100.0;
  int samplesUntilNextBeat = 0;
  int currentStep = 0;
  int lastNote = -1;
  int samplesUntilNoteOff = 0;

  juce::AudioParameterFloat *bpmParam;
  juce::AudioParameterBool *runParam;
  juce::AudioParameterInt *stepParams[8];
  juce::AudioParameterFloat *gateParams[8];
  juce::AudioParameterFloat *filterEnvParams[8];
};
