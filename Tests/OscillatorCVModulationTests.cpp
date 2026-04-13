#include "Modules/OscillatorModule.h"
#include <cmath>
#include <gtest/gtest.h>

class OscillatorCVModulationTest : public ::testing::Test {
protected:
    std::unique_ptr<OscillatorModule> osc;
    static constexpr double sampleRate = 44100.0;
    static constexpr int blockSize = 512;

    void SetUp() override {
        osc = std::make_unique<OscillatorModule>();
        osc->prepareToPlay(sampleRate, blockSize);

        // Send noteOn A4 (440Hz) to set voice 0's lastMidiNote
        juce::AudioBuffer<float> initBuf(14, blockSize);
        initBuf.clear();
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0);
        osc->processBlock(initBuf, midi);
    }

    static float computeRMS(const juce::AudioBuffer<float>& buf, int channel) {
        float sum = 0.0f;
        for (int i = 0; i < buf.getNumSamples(); ++i) {
            float s = buf.getSample(channel, i);
            sum += s * s;
        }
        return std::sqrt(sum / (float)buf.getNumSamples());
    }

    static float estimateFreqByZeroCrossings(const juce::AudioBuffer<float>& buf, int channel, double sr) {
        int crossings = 0;
        for (int i = 1; i < buf.getNumSamples(); ++i) {
            if ((buf.getSample(channel, i - 1) >= 0.0f) != (buf.getSample(channel, i) >= 0.0f))
                ++crossings;
        }
        return (float)(crossings * sr / (2.0 * buf.getNumSamples()));
    }
};

TEST_F(OscillatorCVModulationTest, OctaveCVShiftsFrequency) {
    // Reference block with no CV
    juce::AudioBuffer<float> refBuf(14, blockSize);
    refBuf.clear();
    juce::MidiBuffer emptyMidi;
    osc->processBlock(refBuf, emptyMidi);
    float refFreq = estimateFreqByZeroCrossings(refBuf, 0, sampleRate);

    // Reset oscillator
    osc->prepareToPlay(sampleRate, blockSize);
    juce::AudioBuffer<float> initBuf(14, blockSize);
    initBuf.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0);
    osc->processBlock(initBuf, midi);

    // Octave CV = 0.5 on channel 2 -> octShift = round(0.5 * 4) = 2 octaves up
    juce::AudioBuffer<float> modBuf(14, blockSize);
    modBuf.clear();
    for (int i = 0; i < blockSize; ++i)
        modBuf.setSample(2, i, 0.5f);
    osc->processBlock(modBuf, emptyMidi);
    float modFreq = estimateFreqByZeroCrossings(modBuf, 0, sampleRate);

    // 2 octaves up = ~4x frequency
    EXPECT_GT(modFreq, refFreq * 2.5f) << "Octave CV should shift frequency up significantly";
}

TEST_F(OscillatorCVModulationTest, LevelCVReducesAmplitude) {
    // Reference with default level (1.0)
    juce::AudioBuffer<float> refBuf(14, blockSize);
    refBuf.clear();
    juce::MidiBuffer emptyMidi;
    osc->processBlock(refBuf, emptyMidi);
    float refRMS = computeRMS(refBuf, 0);

    // Level CV = -0.5 on channel 5 -> level = clamp(1.0 + (-0.5)) = 0.5
    juce::AudioBuffer<float> modBuf(14, blockSize);
    modBuf.clear();
    for (int i = 0; i < blockSize; ++i)
        modBuf.setSample(5, i, -0.5f);
    osc->processBlock(modBuf, emptyMidi);
    float modRMS = computeRMS(modBuf, 0);

    EXPECT_LT(modRMS, refRMS * 0.75f) << "Level CV should reduce output amplitude";
    EXPECT_GT(modRMS, 0.01f) << "Output should still have signal";
}

TEST_F(OscillatorCVModulationTest, CoarseCVShiftsFrequency) {
    // Reference
    juce::AudioBuffer<float> refBuf(14, blockSize);
    refBuf.clear();
    juce::MidiBuffer emptyMidi;
    osc->processBlock(refBuf, emptyMidi);
    float refFreq = estimateFreqByZeroCrossings(refBuf, 0, sampleRate);

    // Reset
    osc->prepareToPlay(sampleRate, blockSize);
    juce::AudioBuffer<float> initBuf(14, blockSize);
    initBuf.clear();
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, (juce::uint8)100), 0);
    osc->processBlock(initBuf, midi);

    // Coarse CV = 1.0 on channel 3 -> coarseShift = round(1.0 * 12) = 12 semitones = 1 octave
    juce::AudioBuffer<float> modBuf(14, blockSize);
    modBuf.clear();
    for (int i = 0; i < blockSize; ++i)
        modBuf.setSample(3, i, 1.0f);
    osc->processBlock(modBuf, emptyMidi);
    float modFreq = estimateFreqByZeroCrossings(modBuf, 0, sampleRate);

    EXPECT_GT(modFreq, refFreq * 1.5f) << "Coarse CV should shift frequency up";
}
