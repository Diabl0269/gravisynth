#pragma once

#include "AIProvider.h"
#include "AIStateMapper.h"
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

namespace gsynth {

/**
 * @class AIIntegrationService
 * @brief Orchestrates AI interactions and bridges them with the synth engine.
 */
class AIIntegrationService {
public:
    AIIntegrationService(juce::AudioProcessorGraph& graph);
    ~AIIntegrationService();

    /**
     * @class Listener
     * @brief Interface for observing AI-driven changes to the synthesizer state.
     */
    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void aiPatchApplied() = 0;
    };

    void addListener(Listener* l) { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

    void setProvider(std::unique_ptr<AIProvider> newProvider);

    /**
     * @brief Sends a user message and gets a response.
     */
    void sendMessage(const juce::String& text, AIProvider::CompletionCallback callback);

    /**
     * @brief Applies a JSON patch to the graph.
     */
    bool applyPatch(const juce::String& jsonString);

    /**
     * @brief Returns the current graph state as a JSON string for context.
     */
    juce::String getPatchContext();

    /**
     * @brief Returns the chat history.
     */
    const std::vector<AIProvider::Message>& getHistory() const { return chatHistory; }

    /**
     * @brief Clears the chat history (except system prompt).
     */
    void clearHistory();

private:
    std::unique_ptr<AIProvider> provider;
    std::vector<AIProvider::Message> chatHistory;
    juce::AudioProcessorGraph& audioGraph;
    juce::ListenerList<Listener> listeners;

    void initSystemPrompt();

    JUCE_DECLARE_WEAK_REFERENCEABLE(AIIntegrationService)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIIntegrationService)
};

} // namespace gsynth
