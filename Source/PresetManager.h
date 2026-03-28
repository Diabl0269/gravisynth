#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace gsynth {

/**
 * @class PresetManager
 * @brief Manages built-in "Factory" presets for Gravisynth.
 */
class PresetManager {
public:
    PresetManager();
    ~PresetManager();

    /**
     * @brief Gets a list of available factory preset names.
     */
    static juce::StringArray getPresetNames();

    /**
     * @brief Loads a factory preset by its index.
     * @return true if successful.
     */
    static bool loadPreset(int index, juce::AudioProcessorGraph& graph);

    /**
     * @brief Loads the initial default preset.
     */
    static bool loadDefaultPreset(juce::AudioProcessorGraph& graph);

private:
    static juce::String getPresetJSON(int index);
};

} // namespace gsynth
