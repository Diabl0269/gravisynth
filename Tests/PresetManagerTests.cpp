#include "../Source/PresetManager.h"
#include <gtest/gtest.h>
#include <juce_audio_processors/juce_audio_processors.h>

TEST(PresetManagerTest, ListPresets) {
    auto names = gsynth::PresetManager::getPresetNames();
    ASSERT_GT(names.size(), 0);
    ASSERT_TRUE(names.contains("Default"));
    ASSERT_TRUE(names.contains("Simple Lead"));
    ASSERT_TRUE(names.contains("Ambient Pad"));
    ASSERT_TRUE(names.contains("Modulated Bass"));
    ASSERT_TRUE(names.contains("Step Sequence"));
    ASSERT_TRUE(names.contains("Polyphonic Keys (WIP)"));
}

TEST(PresetManagerTest, LoadAllPresets) {
    juce::AudioProcessorGraph graph;
    auto names = gsynth::PresetManager::getPresetNames();

    for (int i = 0; i < names.size(); ++i) {
        bool success = gsynth::PresetManager::loadPreset(i, graph);
        EXPECT_TRUE(success) << "Failed to load preset: " << names[i].toStdString();
        EXPECT_GT(graph.getNumNodes(), 0) << "No nodes loaded for preset: " << names[i].toStdString();
    }
}

TEST(PresetManagerTest, LoadDefaultPreset) {
    juce::AudioProcessorGraph graph;
    bool success = gsynth::PresetManager::loadDefaultPreset(graph);
    ASSERT_TRUE(success);
    ASSERT_GT(graph.getNumNodes(), 0);
}
