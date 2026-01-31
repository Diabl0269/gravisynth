#include "Modules/FilterModule.h"
#include <gtest/gtest.h>

class FilterTest : public ::testing::Test {
protected:
    FilterModule filter;
    juce::AudioBuffer<float> buffer;
    juce::MidiBuffer midiMessages;

    void SetUp() override {
        filter.prepareToPlay(44100.0, 512);
        buffer.setSize(2, 512);
        // Create some noise or DC offset to filter
        juce::Random r;
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                data[i] = r.nextFloat() * 2.0f - 1.0f;
            }
        }
    }
};

TEST_F(FilterTest, InitState) {
    // Just ensure it processes without crashing immediately
    filter.processBlock(buffer, midiMessages);
}

TEST_F(FilterTest, LowPassAttenuatesHighFreq) {
    // Hard to test exact attenuation without FFT, but we can check RMS change.
    // If we set cutoff very low, output RMS should be significantly lower than
    // input.

    // 1. Measure input RMS
    float inputRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());

    // 2. Set Cutoff Low (e.g., 20Hz)
    // Param index 0 is likely Cutoff based on ModuleBase pattern, but let's find
    // by name if possible or just assume implementation details for now as we are
    // inside the codebase. Let's iterate params to find "Cutoff".

    auto* cutoffParam = filter.getParameters()[0]; // Assumption: 0=Cutoff, 1=Resonance
    for (auto* p : filter.getParameters()) {
        if (p->getName(100) == "Cutoff") {
            cutoffParam = p;
            break;
        }
    }

    // Normalized value 0.0 should be min freq
    cutoffParam->setValueNotifyingHost(0.0f);

    // 3. Process
    filter.processBlock(buffer, midiMessages);

    // 4. Measure output RMS
    float outputRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());

    EXPECT_LT(outputRMS, inputRMS);
}
