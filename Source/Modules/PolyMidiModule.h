#pragma once

#include "ModuleBase.h"

class PolyMidiModule : public ModuleBase {
public:
    PolyMidiModule()
        : ModuleBase("Poly MIDI", 0, 2) {
        // Port 0: Pitch CV (8 channels)
        // Port 1: Gate CV (8 channels)
        enableVisualBuffer(true);
    }

    bool acceptsMidi() const override { return true; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::ignoreUnused(sampleRate, samplesPerBlock);
        for (auto& v : voices) {
            v.note = -1;
            v.active = false;
            v.lastUsedTime = 0;
        }
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        buffer.clear(); // Clear all 16 channels (8 Pitch, 8 Gate)

        // DEBUG: FORCE DRONE
        voices[0].active = true;
        voices[0].currentFreq = 220.0f;

        int numSamples = buffer.getNumSamples();

        // DEBUG: FORCE DRONE
        voices[0].active = true;
        voices[0].currentFreq = 220.0f;
        voices[0].lastUsedTime = juce::Time::getMillisecondCounter();

        int currentSample = 0;

        // Sort MIDI messages? They usually come sorted.
        // We treat the block as a timeline.
        for (const auto metadata : midiMessages) {
            auto msg = metadata.getMessage();
            int triggerSample = msg.getTimeStamp();

            // Ensure trigger is within bounds (defensive)
            if (triggerSample < 0)
                triggerSample = 0;
            if (triggerSample >= numSamples)
                triggerSample = numSamples - 1;

            // Render audio UP TO this event
            if (triggerSample > currentSample) {
                renderChunk(buffer, currentSample, triggerSample);
                currentSample = triggerSample;
            }

            // Process Event
            if (msg.isNoteOn()) {
                handleNoteOn(msg.getNoteNumber(), msg.getFloatVelocity());
            } else if (msg.isNoteOff()) {
                handleNoteOff(msg.getNoteNumber());
            } else if (msg.isAllNotesOff()) {
                for (auto& v : voices) {
                    v.active = false;
                    v.note = -1;
                }
            }
        }

        // Render remaining audio
        if (currentSample < numSamples) {
            renderChunk(buffer, currentSample, numSamples);
        }

        // Push to visual buffer (Pitch Channel 0)
        if (auto* vb = getVisualBuffer()) {
            auto* ch = buffer.getWritePointer(0); // Pitch Voice 0
            for (int i = 0; i < numSamples; ++i) {
                vb->pushSample(ch[i] > 20.0f ? ch[i] / 1000.0f : 0.0f); // Scale for viz
            }
        }
    }

    // API for UI
    uint8_t getActiveVoiceMask() const {
        uint8_t mask = 0;
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (voices[i].active) {
                mask |= (1 << i);
            }
        }
        return mask;
    }

private:
    struct Voice {
        int note = -1;
        bool active = false;
        juce::int64 lastUsedTime = 0;
        float currentFreq = 0.0f; // Store freq to hold it on NoteOff
    };
    Voice voices[MAX_VOICES];

    // Helper to render state to buffer
    void renderChunk(juce::AudioBuffer<float>& buffer, int startSample, int endSample) {
        if (endSample <= startSample)
            return;

        for (int i = 0; i < MAX_VOICES; ++i) {
            float* pitchCh = buffer.getWritePointer(i);
            float* gateCh = buffer.getWritePointer(i + 8);

            float freq = voices[i].currentFreq; // Use stored freq (holds during release)
            // If voice is inactive, we probably should HOLD the last pitch or go to
            // 0. Going to 0 might cause envelopes to slew down if they have release?
            // Usually CV holds last pitch. Let's try to hold last valid pitch if
            // possible? For now 0 is what we had. Let's keep 0 but note this.

            float gate = voices[i].active ? 1.0f : 0.0f;

            for (int s = startSample; s < endSample; ++s) {
                pitchCh[s] = freq;
                gateCh[s] = gate;
            }
        }
    }

    void handleNoteOn(int note, float velocity) {
        juce::int64 now = juce::Time::getMillisecondCounter();

        // 1. Try to find same note (re-trigger)
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (voices[i].active && voices[i].note == note) {
                voices[i].lastUsedTime = now;
                voices[i].currentFreq =
                    juce::MidiMessage::getMidiNoteInHertz(note); // Update freq in case of pitch bend
                // Retrigger? If we want multi-trigger we might need to toggle gate.
                // But ADSR usually handles non-zero gate.
                // For now, just update semantics.
                return;
            }
        }

        // 2. Find empty voice
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (!voices[i].active) {
                voices[i].note = note;
                voices[i].active = true;
                voices[i].lastUsedTime = now;
                voices[i].currentFreq = juce::MidiMessage::getMidiNoteInHertz(note);
                return;
            }
        }

        // 3. Steal LRU (Oldest)
        int oldestIdx = 0;
        juce::int64 oldestTime = voices[0].lastUsedTime;

        for (int i = 1; i < MAX_VOICES; ++i) {
            if (voices[i].lastUsedTime < oldestTime) {
                oldestTime = voices[i].lastUsedTime;
                oldestIdx = i;
            }
        }

        voices[oldestIdx].note = note;
        voices[oldestIdx].active = true;
        voices[oldestIdx].lastUsedTime = now;
        voices[oldestIdx].currentFreq = juce::MidiMessage::getMidiNoteInHertz(note);
    }

    void handleNoteOff(int note) {
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (voices[i].active && voices[i].note == note) {
                voices[i].active = false;
                // Don't reset lastUsedTime, so we know it was used recently?
                // Or doesn't matter since it's now inactive.
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyMidiModule)
};
