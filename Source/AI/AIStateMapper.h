#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>

namespace gsynth {

/**
 * @class AIStateMapper
 * @brief Handles conversion between AI-friendly JSON and juce::AudioProcessorGraph.
 */
class AIStateMapper {
public:
    /**
     * @brief Converts the current graph state to a JSON-compatible juce::var.
     */
    static juce::var graphToJSON(juce::AudioProcessorGraph& graph);

    /**
     * @brief Applies a JSON-compatible juce::var to the graph.
     * @return true if the patch was applied successfully.
     */
    static bool applyJSONToGraph(const juce::var& json, juce::AudioProcessorGraph& graph);

private:
    static std::unique_ptr<juce::AudioProcessor> createModule(const juce::String& type);
};

} // namespace gsynth
