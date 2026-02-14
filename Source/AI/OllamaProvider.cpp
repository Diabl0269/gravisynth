#include "OllamaProvider.h"

namespace gsynth {

void OllamaProvider::setModel(const juce::String& name) { currentModel = name; }
juce::String OllamaProvider::getCurrentModel() const { return currentModel; }

void OllamaProvider::fetchAvailableModels(std::function<void(const juce::StringArray& models, bool success)> callback) {
    // Run on a separate thread to avoid blocking UI
    juce::Thread::launch([this, callback]() {
        juce::Logger::writeToLog("AI Discovery STARTED: " + ollamaHost + "/api/tags");
        juce::URL url(ollamaHost + "/api/tags");
        juce::StringArray models;
        bool success = false;

        if (auto stream = std::unique_ptr<juce::InputStream>(
                url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)))) {
            juce::String responseText = stream->readEntireStreamAsString();
            juce::Logger::writeToLog("AI Discovery Response: " + responseText);
            juce::var jsonResponse = juce::JSON::parse(responseText);

            juce::Logger::writeToLog("AI Discovery Debug: jsonResponse.isObject() = " +
                                     juce::String(jsonResponse.isObject() ? "true" : "false"));
            if (jsonResponse.isObject()) {
                juce::Logger::writeToLog(
                    "AI Discovery Debug: hasProperty(\"models\") = " +
                    juce::String(jsonResponse.getDynamicObject()->hasProperty("models") ? "true" : "false"));
                if (jsonResponse.getDynamicObject()->hasProperty("models")) {
                    auto modelsArray = jsonResponse["models"];
                    juce::Logger::writeToLog("AI Discovery Debug: modelsArray.isArray() = " +
                                             juce::String(modelsArray.isArray() ? "true" : "false"));
                    if (modelsArray.isArray()) {
                        for (int i = 0; i < modelsArray.size(); ++i) {
                            models.add(modelsArray[i]["name"].toString());
                        }
                        success = true;
                        juce::Logger::writeToLog("AI Discovery: Found " + juce::String(models.size()) + " " +
                                                 (models.size() == 1 ? "model" : "models"));
                    } else {
                        juce::Logger::writeToLog("AI Discovery Error: 'models' property is not an array");
                    }
                } else {
                    juce::Logger::writeToLog("AI Discovery Error: JSON response does not have 'models' property");
                }
            } else {
                juce::Logger::writeToLog("AI Discovery Error: JSON response is not an object");
            }
        } else {
            juce::Logger::writeToLog("AI Discovery Error: Failed to open input stream for " + url.toString(true));
        }

        juce::MessageManager::callAsync([callback, models, success]() {
            if (callback)
                callback(models, success);
        });
    });
}

} // namespace gsynth