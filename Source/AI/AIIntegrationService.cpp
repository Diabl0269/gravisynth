#include "AIIntegrationService.h"

namespace gsynth {

AIIntegrationService::AIIntegrationService() { initSystemPrompt(); }

AIIntegrationService::~AIIntegrationService() {}

void AIIntegrationService::setProvider(std::unique_ptr<AIProvider> newProvider) { provider = std::move(newProvider); }

void AIIntegrationService::sendMessage(const juce::String& text, AIProvider::CompletionCallback callback) {
    chatHistory.push_back({"user", text});

    if (provider) {
        provider->sendPrompt(chatHistory, [this, callback](const juce::String& response, bool success) {
            if (success) {
                chatHistory.push_back({"assistant", response});
            }
            if (callback) {
                callback(response, success);
            }
        });
    } else {
        if (callback) {
            callback("Error: No AI provider selected.", false);
        }
    }
}

void AIIntegrationService::clearHistory() {
    chatHistory.clear();
    initSystemPrompt();
}

void AIIntegrationService::initSystemPrompt() {
    chatHistory.push_back({"system",
                           "You are Gravisynth AI, an expert sound designer for the Gravisynth modular synthesizer. "
                           "Your goal is to help users create and modify patches. "
                           "When asked to create a patch, provide a JSON description of the modules and connections. "
                           "Keep your explanations concise."});
}

} // namespace gsynth
