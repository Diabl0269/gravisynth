#pragma once

#include "AIProvider.h"
#include <functional>
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
    AIIntegrationService();
    ~AIIntegrationService();

    void setProvider(std::unique_ptr<AIProvider> newProvider);

    /**
     * @brief Sends a user message and gets a response.
     */
    void sendMessage(const juce::String& text, AIProvider::CompletionCallback callback);

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

    void initSystemPrompt();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIIntegrationService)
};

} // namespace gsynth
