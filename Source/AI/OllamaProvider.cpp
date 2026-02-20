#include "OllamaProvider.h"

namespace gsynth {

// Default constructor implementation
OllamaProvider::OllamaProvider(const juce::String& host)
    : Thread("OllamaProviderThread")
    , ollamaHost(host)
    , createStream([](const juce::URL& url, const juce::URL::InputStreamOptions& options) {
        return url.createInputStream(options);
    }) {}

void OllamaProvider::setModel(const juce::String& name) { currentModel = name; }
juce::String OllamaProvider::getCurrentModel() const { return currentModel; }

void OllamaProvider::fetchAvailableModels(std::function<void(const juce::StringArray& models, bool success)> callback) {
    // Run on a separate thread to avoid blocking UI
    juce::Thread::launch([this, callback]() {
        DBG("AI Discovery STARTED: " + ollamaHost + "/api/tags");
        juce::URL url(ollamaHost + "/api/tags");
        juce::StringArray models;
        bool success = false;

        if (auto stream = createStream(url, juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                .withConnectionTimeoutMs(120000))) {
            juce::String responseText = stream->readEntireStreamAsString();
            // DBG("AI Discovery Response (before JSON parse): [" + responseText + "]"); // Potentially expensive
            // DBG("AI Discovery Response: " + responseText); // Redundant

            juce::var jsonResponse = juce::JSON::parse(responseText);

            DBG("AI Discovery Debug: jsonResponse.isObject() = " +
                juce::String(jsonResponse.isObject() ? "true" : "false"));
            if (jsonResponse.isObject()) {
                DBG("AI Discovery Debug: hasProperty(\"models\") = " +
                    juce::String(jsonResponse.getDynamicObject()->hasProperty("models") ? "true" : "false"));
                if (jsonResponse.getDynamicObject()->hasProperty("models")) {
                    auto modelsArray = jsonResponse["models"];
                    DBG("AI Discovery Debug: modelsArray.isArray() = " +
                        juce::String(modelsArray.isArray() ? "true" : "false"));
                    if (modelsArray.isArray()) {
                        for (int i = 0; i < modelsArray.size(); ++i) {
                            models.add(modelsArray[i]["name"].toString());
                        }
                        success = true;
                        DBG("AI Discovery: Found " + juce::String(models.size()) + " " +
                            (models.size() == 1 ? "model" : "models"));
                    } else {
                        DBG("AI Discovery Error: 'models' property is not an array");
                    }
                } else {
                    DBG("AI Discovery Error: JSON response does not have 'models' property");
                }
            } else {
                DBG("AI Discovery Error: JSON response is not an object");
            }
        } else {
            DBG("AI Discovery Error: Failed to open input stream for " + url.toString(true));
        }

        if (isTestMode) {
            if (callback)
                callback(models, success);
        } else {
            juce::MessageManager::callAsync([callback, models, success]() {
                if (callback)
                    callback(models, success);
            });
        }
    });
}

void OllamaProvider::run() {
    while (!threadShouldExit()) {
        Request currentRequest;

        {
            const juce::ScopedLock sl(queueLock);
            if (pendingRequests.empty()) {
                break;
            }
            currentRequest = pendingRequests.front();
            pendingRequests.erase(pendingRequests.begin());
        }

        processRequest(currentRequest);
    }
}

void OllamaProvider::processRequest(const Request& req) {
    juce::URL url(ollamaHost + "/api/chat");

    // Build JSON body
    juce::DynamicObject::Ptr body = new juce::DynamicObject();
    body->setProperty("model", currentModel);
    body->setProperty("stream", false);

    juce::Array<juce::var> messages;
    for (const auto& msg : req.conversation) {
        juce::DynamicObject::Ptr m = new juce::DynamicObject();
        m->setProperty("role", msg.role);
        m->setProperty("content", msg.content);
        messages.add(juce::var(m.get()));
    }
    body->setProperty("messages", messages);

    if (!req.responseSchema.isVoid())
        body->setProperty("format", req.responseSchema);

    juce::String jsonString = juce::JSON::toString(juce::var(body.get()));

    juce::String responseText;
    bool success = false;

    if (auto stream = createStream(
            url.withPOSTData(jsonString),
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData).withConnectionTimeoutMs(120000))) {
        responseText = stream->readEntireStreamAsString();
        // DBG("AI Chat Response (before JSON parse): [" + responseText + "]"); // Potentially expensive

        juce::var jsonResponse = juce::JSON::parse(responseText);
        if (jsonResponse.isObject()) {
            if (auto* obj = jsonResponse.getDynamicObject()) {
                if (obj->hasProperty("message")) {
                    auto msgObj = obj->getProperty("message");
                    if (msgObj.getDynamicObject()) {
                        responseText = msgObj.getDynamicObject()->getProperty("content").toString();
                        success = true;
                    }
                }
            }
        }
    } else {
        responseText = "Error: Could not connect to Ollama at " + ollamaHost;
    }

    if (isTestMode) {
        if (req.callback)
            req.callback(responseText, success);
    } else {
        juce::MessageManager::callAsync([req, responseText, success]() {
            if (req.callback)
                req.callback(responseText, success);
        });
    }
}

} // namespace gsynth