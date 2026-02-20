#include "AIIntegrationService.h"

namespace gsynth {

AIIntegrationService::AIIntegrationService(juce::AudioProcessorGraph& graph)
    : audioGraph(graph) {
    initSystemPrompt();
}

AIIntegrationService::~AIIntegrationService() {}

void AIIntegrationService::setProvider(std::unique_ptr<AIProvider> newProvider) { provider = std::move(newProvider); }

void AIIntegrationService::sendMessage(const juce::String& text, AIProvider::CompletionCallback callback,
                                       bool useStructuredOutput) {
    // Before sending, we could inject the current patch context as a developer message or hidden part of prompt
    // For now, let's just add the user text.
    chatHistory.push_back({"user", text});

    if (provider) {
        auto weakThis = juce::WeakReference<AIIntegrationService>(this);
        auto schema = useStructuredOutput ? AIStateMapper::getPatchSchema() : juce::var();

        provider->sendPrompt(
            chatHistory,
            [weakThis, callback](const juce::String& response, bool success) {
                if (weakThis.get() == nullptr)
                    return; // Service was destroyed

                auto* self = weakThis.get();
                if (success) {
                    self->chatHistory.push_back({"assistant", response});
                }
                if (callback) {
                    callback(response, success);
                }
            },
            schema);
    } else {
        if (callback) {
            callback("Error: No AI provider selected.", false);
        }
    }
}

bool AIIntegrationService::applyPatch(const juce::String& jsonString) {
    juce::String extractedJson = extractJsonFromResponse(jsonString);
    juce::var json = juce::JSON::parse(extractedJson);
    if (AIStateMapper::applyJSONToGraph(json, audioGraph)) {
        listeners.call([](Listener& l) { l.aiPatchApplied(); });
        return true;
    }
    return false;
}

juce::String AIIntegrationService::extractJsonFromResponse(const juce::String& response) {
    // 1. Try to find JSON between backticks
    int start = response.indexOf("```json");
    if (start != -1) {
        start += 7;
        int end = response.indexOf(start, "```");
        if (end != -1)
            return response.substring(start, end).trim();
    }

    // 2. Try to find JSON between any backticks
    start = response.indexOf("```");
    if (start != -1) {
        start += 3;
        int end = response.indexOf(start, "```");
        if (end != -1)
            return response.substring(start, end).trim();
    }

    // 3. Try to find first '{' and last '}'
    start = response.indexOf("{");
    int end = response.lastIndexOf("}");
    if (start != -1 && end != -1 && end > start)
        return response.substring(start, end + 1).trim();

    return response.trim();
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
    juce::String schema = AIStateMapper::getModuleSchema();

    juce::String systemMsg =
        "You are Gravisynth AI, an expert sound designer for the Gravisynth modular synthesizer. "
        "Your goal is to help users create and modify patches. "
        "Gravisynth uses a nodes-and-connections model. "
        "\n\n" +
        schema +
        "\n"
        "### MODES OF OPERATION:\n"
        "1. **Conversational Mode**: When the user asks a general question, respond naturally in Markdown.\n"
        "2. **Structured Patch Mode**: When requested to create or modify a patch, you MUST provide a JSON block. "
        "If a 'format' schema is provided in the API request, your entire response MUST be the raw JSON adhering to "
        "that schema, with NO additional text or Markdown formatting.\n"
        "\n"
        "### IMPORTANT INSTRUCTIONS FOR PATCHES:\n"
        "- **Parameter IDs are Case-Sensitive**: Use the exact `Parameter ID` from the table above (e.g., use "
        "`cutoff`, not `Cutoff`).\n"
        "- **Values**: Use raw, unnormalized values within the specified ranges.\n"
        "- **Choice Parameters**: Use the exact string name (e.g., `\"waveform\": \"Saw\"`).\n"
        "- **Connections**: Ensure `srcPort` and `dstPort` are valid for the given module type. Most modules use port "
        "0 for their primary audio/midi signal.\n"
        "\nExample format:\n"
        "```json\n"
        "{\n"
        "  \"nodes\": [\n"
        "    { \"id\": 1, \"type\": \"Oscillator\", \"params\": { \"frequency\": 440.0, \"waveform\": \"Saw\" } },\n"
        "    { \"id\": 2, \"type\": \"Audio Output\" }\n"
        "  ],\n"
        "  \"connections\": [\n"
        "    { \"src\": 1, \"srcPort\": 0, \"dst\": 2, \"dstPort\": 0 }\n"
        "  ]\n"
        "}\n"
        "```";

    chatHistory.push_back({"system", systemMsg});
}

void AIIntegrationService::setModel(const juce::String& name) {
    if (provider)
        provider->setModel(name);
}

juce::String AIIntegrationService::getCurrentModel() const { return provider ? provider->getCurrentModel() : ""; }

void AIIntegrationService::fetchAvailableModels(
    std::function<void(const juce::StringArray& models, bool success)> callback) {
    if (provider)
        provider->fetchAvailableModels(callback);
    else if (callback)
        callback({}, false);
}
} // namespace gsynth
