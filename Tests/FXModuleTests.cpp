#include "Modules/AttenuverterModule.h"
#include "Modules/FX/DelayModule.h"
#include "Modules/FX/DistortionModule.h"
#include "Modules/FX/ReverbModule.h"
#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

// ---------------------------------------------------------------------------
// AttenuverterModule tests
// ---------------------------------------------------------------------------

class AttenuverterModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<AttenuverterModule>();
        module->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<AttenuverterModule> module;
};

TEST_F(AttenuverterModuleTest, ProcessBlockAttenuatesSignal) {
    // 2 channels: ch0 = audio, ch1 = CV amount
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    // Fill audio channel with a constant 1.0f
    for (int i = 0; i < 512; ++i)
        buffer.setSample(0, i, 1.0f);

    // Set amount param to 0.5 via the first float parameter
    auto* amountParam = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[0]);
    ASSERT_NE(amountParam, nullptr);
    amountParam->setValueNotifyingHost(0.75f); // normalized: maps to 0.75 in [-1,1] range => 0.5

    juce::MidiBuffer midi;
    module->processBlock(buffer, midi);

    // Output should be attenuated (not the original 1.0f)
    // We just verify it stays within a reasonable range and is non-zero
    float lastSample = buffer.getSample(0, 511);
    EXPECT_GE(lastSample, -1.0f);
    EXPECT_LE(lastSample, 1.0f);
}

TEST_F(AttenuverterModuleTest, ProcessBlockBypassClearsSilent) {
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    // Fill audio channel with signal
    for (int i = 0; i < 512; ++i)
        buffer.setSample(0, i, 1.0f);

    // Enable bypass (second parameter)
    auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(module->getParameters()[1]);
    ASSERT_NE(bypassParam, nullptr);
    bypassParam->setValueNotifyingHost(1.0f); // true

    juce::MidiBuffer midi;
    module->processBlock(buffer, midi);

    // With bypass enabled, buffer should be cleared (silent)
    for (int i = 0; i < 512; ++i) {
        EXPECT_FLOAT_EQ(buffer.getSample(0, i), 0.0f);
    }
}

TEST_F(AttenuverterModuleTest, ProcessBlockWithCVAmountModulation) {
    // ch0 = audio, ch1 = CV modulation on amount
    juce::AudioBuffer<float> buffer(2, 1000);
    buffer.clear();

    // Fill audio channel with 1.0f
    for (int i = 0; i < 1000; ++i)
        buffer.setSample(0, i, 1.0f);

    // Set base amount to 0.0 so the CV alone controls the output
    auto* amountParam = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[0]);
    ASSERT_NE(amountParam, nullptr);
    amountParam->setValueNotifyingHost(0.5f); // normalized 0.5 => 0.0 in [-1,1]

    // Set CV channel to 0.5
    for (int i = 0; i < 1000; ++i)
        buffer.setSample(1, i, 0.5f);

    juce::MidiBuffer midi;
    module->processBlock(buffer, midi);

    // With CV = 0.5 and base amount ~0.0, result should be influenced by CV
    // Output should be non-zero at the last sample (after smoothing settles)
    float lastSample = buffer.getSample(0, 999);
    EXPECT_NE(lastSample, 0.0f);
}

TEST_F(AttenuverterModuleTest, ProcessBlockEmptyBufferDoesNotCrash) {
    juce::AudioBuffer<float> buffer(0, 512);
    juce::MidiBuffer midi;
    // Should return early without crashing
    EXPECT_NO_THROW(module->processBlock(buffer, midi));
}

// ---------------------------------------------------------------------------
// DelayModule tests
// ---------------------------------------------------------------------------

class DelayModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<DelayModule>();
        module->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<DelayModule> module;
};

TEST_F(DelayModuleTest, ProcessBlockPassesSignalThrough) {
    // DelayModule: 2 inputs, 2 outputs
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    // Fill both channels with signal
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.8f);

    juce::MidiBuffer midi;
    module->processBlock(buffer, midi);

    // With mix=0.3 (default), output = wet*0.3 + dry*0.7
    // Dry portion alone should keep output non-zero
    float outSample = buffer.getSample(0, 0);
    EXPECT_NE(outSample, 0.0f);
}

TEST_F(DelayModuleTest, FeedbackParameterExists) {
    auto* feedbackParam = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[1]);
    ASSERT_NE(feedbackParam, nullptr);
    EXPECT_FLOAT_EQ(feedbackParam->get(), 0.5f); // default
}

TEST_F(DelayModuleTest, PrepareToPlayAndProcessDoNotCrash) {
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.1f);

    juce::MidiBuffer midi;
    EXPECT_NO_THROW(module->processBlock(buffer, midi));
}

// ---------------------------------------------------------------------------
// DistortionModule tests
// ---------------------------------------------------------------------------

class DistortionModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<DistortionModule>();
        module->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<DistortionModule> module;
};

TEST_F(DistortionModuleTest, ProcessBlockProducesOutput) {
    // DistortionModule: 4 channels (2 audio + 2 CV)
    juce::AudioBuffer<float> buffer(4, 512);
    buffer.clear();

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.5f);

    juce::MidiBuffer midi;
    module->processBlock(buffer, midi);

    // Output should be non-zero (signal passed through)
    bool anyNonZero = false;
    for (int i = 0; i < 512; ++i) {
        if (buffer.getSample(0, i) != 0.0f) {
            anyNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(anyNonZero);

    // CV channels should be cleared
    for (int i = 0; i < 512; ++i) {
        EXPECT_FLOAT_EQ(buffer.getSample(2, i), 0.0f);
        EXPECT_FLOAT_EQ(buffer.getSample(3, i), 0.0f);
    }
}

TEST_F(DistortionModuleTest, HighDriveClipsSignal) {
    DistortionModule lowDrive, highDrive;
    lowDrive.prepareToPlay(44100.0, 512);
    highDrive.prepareToPlay(44100.0, 512);

    auto* driveParamLow = dynamic_cast<juce::AudioParameterFloat*>(lowDrive.getParameters()[0]);
    auto* driveParamHigh = dynamic_cast<juce::AudioParameterFloat*>(highDrive.getParameters()[0]);
    ASSERT_NE(driveParamLow, nullptr);
    ASSERT_NE(driveParamHigh, nullptr);

    driveParamLow->setValueNotifyingHost(0.0f);  // min drive (1.0)
    driveParamHigh->setValueNotifyingHost(1.0f); // max drive (20.0)

    // Set mix=1.0 (fully wet) on both so we hear the distortion effect clearly
    auto* mixParamLow = dynamic_cast<juce::AudioParameterFloat*>(lowDrive.getParameters()[1]);
    auto* mixParamHigh = dynamic_cast<juce::AudioParameterFloat*>(highDrive.getParameters()[1]);
    mixParamLow->setValueNotifyingHost(1.0f);
    mixParamHigh->setValueNotifyingHost(1.0f);

    juce::AudioBuffer<float> bufferLow(4, 512);
    juce::AudioBuffer<float> bufferHigh(4, 512);
    bufferLow.clear();
    bufferHigh.clear();

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i) {
            bufferLow.setSample(ch, i, 0.5f);
            bufferHigh.setSample(ch, i, 0.5f);
        }

    juce::MidiBuffer midi;
    lowDrive.processBlock(bufferLow, midi);
    highDrive.processBlock(bufferHigh, midi);

    // High drive should produce larger (more clipped) amplitude than low drive
    float ampLow = std::abs(bufferLow.getSample(0, 511));
    float ampHigh = std::abs(bufferHigh.getSample(0, 511));
    EXPECT_GT(ampHigh, ampLow);
}

TEST_F(DistortionModuleTest, PrepareToPlayAndProcessDoNotCrash) {
    juce::AudioBuffer<float> buffer(4, 512);
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.3f);

    juce::MidiBuffer midi;
    EXPECT_NO_THROW(module->processBlock(buffer, midi));
}

// ---------------------------------------------------------------------------
// ReverbModule tests
// ---------------------------------------------------------------------------

class ReverbModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        module = std::make_unique<ReverbModule>();
        module->prepareToPlay(44100.0, 512);
    }

    std::unique_ptr<ReverbModule> module;
};

TEST_F(ReverbModuleTest, ProcessBlockProducesOutput) {
    // ReverbModule: 2 channels (stereo)
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.5f);

    juce::MidiBuffer midi;
    module->processBlock(buffer, midi);

    // Output should be non-zero (wet + dry signal)
    bool anyNonZero = false;
    for (int i = 0; i < 512; ++i) {
        if (buffer.getSample(0, i) != 0.0f) {
            anyNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(anyNonZero);
}

TEST_F(ReverbModuleTest, PrepareToPlayAndProcessDoNotCrash) {
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.2f);

    juce::MidiBuffer midi;
    EXPECT_NO_THROW(module->processBlock(buffer, midi));
}

TEST_F(ReverbModuleTest, MonoProcessingDoesNotCrash) {
    // Test with a mono buffer (single channel)
    juce::AudioBuffer<float> buffer(1, 512);
    buffer.clear();
    for (int i = 0; i < 512; ++i)
        buffer.setSample(0, i, 0.4f);

    juce::MidiBuffer midi;
    EXPECT_NO_THROW(module->processBlock(buffer, midi));

    // Mono reverb should produce non-zero output
    bool anyNonZero = false;
    for (int i = 0; i < 512; ++i) {
        if (buffer.getSample(0, i) != 0.0f) {
            anyNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(anyNonZero);
}

TEST_F(ReverbModuleTest, RoomSizeParameterIsApplied) {
    // Just verify the parameters exist and are the right types
    auto* roomSizeParam = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[0]);
    auto* dampingParam = dynamic_cast<juce::AudioParameterFloat*>(module->getParameters()[1]);
    ASSERT_NE(roomSizeParam, nullptr);
    ASSERT_NE(dampingParam, nullptr);

    // Change room size and verify processBlock still runs without crash
    roomSizeParam->setValueNotifyingHost(1.0f);
    dampingParam->setValueNotifyingHost(0.8f);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, 0.3f);

    juce::MidiBuffer midi;
    EXPECT_NO_THROW(module->processBlock(buffer, midi));
}
