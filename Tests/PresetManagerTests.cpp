#include "../Source/PresetManager.h"
#include <gtest/gtest.h>

TEST(PresetManagerTest, ListPresets) {
    auto names = gsynth::PresetManager::getPresetNames();
    ASSERT_EQ(names.size(), 7);
    EXPECT_EQ(names[0], "Default");
    EXPECT_EQ(names[1], "Simple Lead");
}

TEST(PresetManagerTest, LoadAllPresets) {
    juce::AudioProcessorGraph graph;
    for (int i = 0; i < gsynth::PresetManager::getPresetNames().size(); ++i) {
        graph.clear();
        EXPECT_TRUE(gsynth::PresetManager::loadPreset(i, graph)) << "Failed to load preset " << i;
        EXPECT_GT(graph.getNumNodes(), 0) << "Preset " << i << " loaded with no nodes";
    }
}

TEST(PresetManagerTest, LoadDefaultPreset) {
    juce::AudioProcessorGraph graph;
    EXPECT_TRUE(gsynth::PresetManager::loadDefaultPreset(graph));
    EXPECT_GT(graph.getNumNodes(), 0);
}

TEST(PresetManagerTest, InvalidPresetIndexReturnsFailure) {
    juce::AudioProcessorGraph graph;
    EXPECT_FALSE(gsynth::PresetManager::loadPreset(-1, graph));
    EXPECT_FALSE(gsynth::PresetManager::loadPreset(100, graph));
}

TEST(PresetManagerTest, PresetCategoriesAreValid) {
    auto presets = gsynth::PresetManager::getPresetList();
    auto categories = gsynth::PresetManager::getCategories();
    ASSERT_GT(presets.size(), 0);
    ASSERT_GT(categories.size(), 0);
    for (const auto& preset : presets) {
        EXPECT_TRUE(categories.contains(preset.category))
            << "Preset '" << preset.name.toStdString() << "' has unknown category '" << preset.category.toStdString()
            << "'";
    }
}

TEST(PresetManagerTest, DefaultPresetHasExpectedNodes) {
    juce::AudioProcessorGraph graph;
    gsynth::PresetManager::loadDefaultPreset(graph);
    // Default preset should have at minimum: Audio I/O + Oscillator + Filter + VCA
    EXPECT_GE(graph.getNumNodes(), 5);
}

TEST(PresetManagerTest, AllPresetsHaveAudioOutput) {
    juce::AudioProcessorGraph graph;
    for (int i = 0; i < gsynth::PresetManager::getPresetNames().size(); ++i) {
        graph.clear();
        gsynth::PresetManager::loadPreset(i, graph);
        bool hasOutput = false;
        for (auto* node : graph.getNodes()) {
            if (node->getProcessor()->getName() == "Audio Output")
                hasOutput = true;
        }
        EXPECT_TRUE(hasOutput) << "Preset " << i << " missing Audio Output";
    }
}

TEST(PresetManagerTest, AllPresetsHaveConnections) {
    juce::AudioProcessorGraph graph;
    for (int i = 0; i < gsynth::PresetManager::getPresetNames().size(); ++i) {
        graph.clear();
        gsynth::PresetManager::loadPreset(i, graph);
        EXPECT_GT(graph.getConnections().size(), 0) << "Preset " << i << " has no connections";
    }
}

TEST(PresetManagerTest, PresetNamesMatchPresetList) {
    auto names = gsynth::PresetManager::getPresetNames();
    auto presets = gsynth::PresetManager::getPresetList();
    ASSERT_EQ(names.size(), presets.size());
    for (int i = 0; i < names.size(); ++i) {
        EXPECT_EQ(names[i], presets[i].name);
    }
}
