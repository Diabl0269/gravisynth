#pragma once

#include "AIProvider.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>

namespace gsynth {

/**
 * @class OllamaProvider
 * @brief AI provider implementation for local Ollama instances.
 */
class OllamaProvider
    : public AIProvider
    , private juce::Thread {
public:
    OllamaProvider(const juce::String& host = "http://localhost:11434")
        : Thread("OllamaProviderThread")
        , ollamaHost(host) {}

    ~OllamaProvider() override { stopThread(2000); }

    void sendPrompt(const std::vector<Message>& conversation, CompletionCallback callback) override {
        {
            const juce::ScopedLock sl(queueLock);
            pendingRequests.push_back({conversation, callback});
        }

        if (!isThreadRunning())
            startThread();
    }

    juce::String getProviderName() const override { return "Ollama"; }

    void setModel(const juce::String& name) override;
    juce::String getCurrentModel() const override;
    void fetchAvailableModels(std::function<void(const juce::StringArray& models, bool success)> callback) override;

private:
    juce::String ollamaHost;
    juce::String currentModel = "qwen3-coder-next:latest";

    struct Request {
        std::vector<Message> conversation;
        AIProvider::CompletionCallback callback;
    };

    juce::CriticalSection queueLock;
    std::vector<Request> pendingRequests;

    void run() override {
        while (!threadShouldExit()) {
            Request currentRequest;

            {
                const juce::ScopedLock sl(queueLock);
                if (pendingRequests.empty()) {
                    // Note: We don't stop the thread here if we want it to stay alive,
                    // but for this implementation we'll exit when idle.
                    break;
                }
                currentRequest = pendingRequests.front();
                pendingRequests.erase(pendingRequests.begin());
            }

            processRequest(currentRequest);
        }
    }

    void processRequest(const Request& req) {
        juce::URL url(ollamaHost + "/api/chat");

        // Build JSON body
        juce::DynamicObject::Ptr body = new juce::DynamicObject();
        body->setProperty("model", "qwen3-coder-next:latest"); // Default model
        body->setProperty("stream", false);

        juce::Array<juce::var> messages;
        for (const auto& msg : req.conversation) {
            juce::DynamicObject::Ptr m = new juce::DynamicObject();
            m->setProperty("role", msg.role);
            m->setProperty("content", msg.content);
            messages.add(juce::var(m.get()));
        }
        body->setProperty("messages", messages);

        juce::String jsonString = juce::JSON::toString(juce::var(body.get()));

        juce::String responseText;
        bool success = false;

        if (auto stream = std::unique_ptr<juce::InputStream>(
                url.withPOSTData(jsonString)
                    .createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)))) {
            responseText = stream->readEntireStreamAsString();

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

        juce::MessageManager::callAsync([req, responseText, success]() {
            if (req.callback)
                req.callback(responseText, success);
        });
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OllamaProvider)
};

} // namespace gsynth
