#include "AIIntegrationService.h"

namespace gsynth {

AIIntegrationService::AIIntegrationService(juce::AudioProcessorGraph& graph)
    : audioGraph(graph) {
    initSystemPrompt();
}

AIIntegrationService::~AIIntegrationService() {}

void AIIntegrationService::setProvider(std::unique_ptr<AIProvider> newProvider) { provider = std::move(newProvider); }

void AIIntegrationService::sendMessage(const juce::String& text, AIProvider::CompletionCallback callback) {
    // Before sending, we could inject the current patch context as a developer message or hidden part of prompt
    // For now, let's just add the user text.
    chatHistory.push_back({"user", text});

    if (provider) {
        auto weakThis = juce::WeakReference<AIIntegrationService>(this);
        provider->sendPrompt(chatHistory, [weakThis, callback](const juce::String& response, bool success) {
            if (weakThis.get() == nullptr)
                return; // Service was destroyed

            auto* self = weakThis.get();
            if (success) {
                self->chatHistory.push_back({"assistant", response});
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

bool AIIntegrationService::applyPatch(const juce::String& jsonString) {
    juce::var json = juce::JSON::parse(jsonString);
    if (AIStateMapper::applyJSONToGraph(json, audioGraph)) {
        listeners.call([](Listener& l) { l.aiPatchApplied(); });
        return true;
    }
    return false;
}

juce::String AIIntegrationService::getPatchContext() {
    juce::var json = AIStateMapper::graphToJSON(audioGraph);
    return juce::JSON::toString(json);
}

void AIIntegrationService::clearHistory() {
    chatHistory.clear();
    initSystemPrompt();
}

void AIIntegrationService::initSystemPrompt() {
    chatHistory.push_back(
        {"system", "You are Gravisynth AI, an expert sound designer for the Gravisynth modular synthesizer. "
                   "Your goal is to help users create and modify patches. "
                   "Gravisynth uses a nodes-and-connections model. "
                   "When asked to create a patch, provide a JSON block inside backticks that describes the nodes and "
                   "connections. "
                   "Example format:\n"
                   "```json\n"
                   "{\n"
                   "  \"nodes\": [\n"
                   "    { \"id\": 1, \"type\": \"Oscillator\", \"params\": { \"Frequency\": 0.2 } },\n"
                   "    { \"id\": 2, \"type\": \"Audio Output\" }\n"
                   "  ],\n"
                   "  \"connections\": [\n"
                   "    { \"src\": 1, \"srcPort\": 0, \"dst\": 2, \"dstPort\": 0 }\n"
                   "  ]\n"
                   "}\n"
                   "```\n"
                   "Available modules: Oscillator, Filter, LFO, ADSR, VCA, Sequencer, Distortion, Delay, Reverb. "
                   "IO nodes: Audio Input, Audio Output, Midi Input. "
                   "Keep your explanations concise."});
}

} // namespace gsynth
