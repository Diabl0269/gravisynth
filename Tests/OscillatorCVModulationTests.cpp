#include "Modules/OscillatorModule.h"
#include <gtest/gtest.h>
#include <cmath>
#include <numeric>

class OscillatorCVModulationTest : public ::testing::Test {
protected:
    std::unique_ptr<OscillatorModule> osc;
    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 512;

    void SetUp() override {
        osc = std::make_unique<OscillatorModule>();
        osc->prepareToPlay(sampleRate, blockSize);

        // Set a known MIDI note so we get a predictable frequency
        // Process a block with a noteOn to set voice 0's lastMidiNote
        juce::AudioBuffer<float> initBuf(14, blockSize);
        initBuf.clear();
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0); // A4 = 440Hz
        osc->processBlock(initBuf, midi);
    }
};

// Helper: compute RMS of a buffer channel
static float computeRMS(const juce::AudioBuffer<float>& buf, int channel) {
    float sum = 0.0f;
    for (int i = 0; i < buf.getNumSamples(); ++i) {
        float s = buf.getSample(channel, i);
        sum += s * s;
    }
    return std::sqrt(sum / buf.getNumSamples());
}

// Helper: estimate fundamental frequency by counting zero crossings
static float estimateFreqByZeroCrossings(const juce::AudioBuffer<float>& buf, int channel, double sampleRate) {
    int crossings = 0;
    for (int i = 1; i < buf.getNumSamples(); ++i) {
        if ((buf.getSample(channel, i - 1) >= 0.0f) != (buf.getSample(channel, i) >= 0.0f))
            ++crossings;
    }
    // Each full cycle has 2 zero crossings
    return (float)(crossings * sampleRate / (2.0 * buf.getNumSamples()));
}

TEST_F(OscillatorCVModulationTest, OctaveCVShiftsFrequency) {
    // Generate a reference block with no CV modulation
    juce::AudioBuffer<float> refBuf(14, blockSize);
    refBuf.clear();
    juce::MidiBuffer midi;
    osc->processBlock(refBuf, midi);
    float refFreq = estimateFreqByZeroCrossings(refBuf, 0, sampleRate);

    // Reset oscillator phase by re-preparing
    osc->prepareToPlay(sampleRate, blockSize);
    // Re-send noteOn
    juce::AudioBuffer<float> initBuf(14, blockSize);
    initBuf.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0);
    osc->processBlock(initBuf, midi);

    // Now generate with Octave CV = +1.0 on channel 2 (should shift +4 octaves worth * CV)
    // The code does: octShift = round(cvOctaveCh[i] * 4.0f) => with CV=0.5, octShift=2
    juce::AudioBuffer<float> modBuf(14, blockSize);
    modBuf.clear();
    for (int i = 0; i < blockSize; ++i)
        modBuf.setSample(2, i, 0.5f); // CV = 0.5 -> 2 octaves up
    juce::MidiBuffer emptyMidi;
    osc->processBlock(modBuf, emptyMidi);
    float modFreq = estimateFreqByZeroCrossings(modBuf, 0, sampleRate);

    // With 2 octaves up, frequency should be ~4x the reference
    // Allow generous tolerance since zero-crossing estimation is approximate
    EXPECT_GT(modFreq, refFreq * 2.5f) << "Octave CV should shift frequency up significantly";
}

TEST_F(OscillatorCVModulationTest, LevelCVReducesAmplitude) {
    // Generate reference with default level (1.0)
    juce::AudioBuffer<float> refBuf(14, blockSize);
    refBuf.clear();
    juce::MidiBuffer midi;
    osc->processBlock(refBuf, midi);
    float refRMS = computeRMS(refBuf, 0);

    // Generate with Level CV = -0.5 on channel 5
    // The code does: level = jlimit(0, 1, level + cvLevelCh[i])
    // Default level=1.0, so level + (-0.5) = 0.5
    juce::AudioBuffer<float> modBuf(14, blockSize);
    modBuf.clear();
    for (int i = 0; i < blockSize; ++i)
        modBuf.setSample(5, i, -0.5f); // Level CV = -0.5
    juce::MidiBuffer emptyMidi;
    osc->processBlock(modBuf, emptyMidi);
    float modRMS = computeRMS(modBuf, 0);

    // With level reduced to 0.5, RMS should be roughly half
    EXPECT_LT(modRMS, refRMS * 0.75f) << "Level CV should reduce output amplitude";
    EXPECT_GT(modRMS, 0.01f) << "Output should still have signal";
}

TEST_F(OscillatorCVModulationTest, CoarseCVShiftsFrequency) {
    // Generate reference
    juce::AudioBuffer<float> refBuf(14, blockSize);
    refBuf.clear();
    juce::MidiBuffer midi;
    osc->processBlock(refBuf, midi);
    float refFreq = estimateFreqByZeroCrossings(refBuf, 0, sampleRate);

    // Reset
    osc->prepareToPlay(sampleRate, blockSize);
    juce::AudioBuffer<float> initBuf(14, blockSize);
    initBuf.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0);
    osc->processBlock(initBuf, midi);

    // Coarse CV = 1.0 on channel 3 -> coarseShift = round(1.0 * 12) = 12 semitones = 1 octave
    juce::AudioBuffer<float> modBuf(14, blockSize);
    modBuf.clear();
    for (int i = 0; i < blockSize; ++i)
        modBuf.setSample(3, i, 1.0f);
    juce::MidiBuffer emptyMidi;
    osc->processBlock(modBuf, emptyMidi);
    float modFreq = estimateFreqByZeroCrossings(modBuf, 0, sampleRate);

    // 12 semitones = 1 octave = 2x frequency
    EXPECT_GT(modFreq, refFreq * 1.5f) << "Coarse CV should shift frequency up";
}
