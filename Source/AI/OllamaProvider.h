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
    , protected juce::Thread {
public:
    using InputStreamFactory =
        std::function<std::unique_ptr<juce::InputStream>(const juce::URL&, const juce::URL::InputStreamOptions&)>;

    OllamaProvider(const juce::String& host = "http://localhost:11434");

    // Test-specific constructor to inject a mock input stream factory
    OllamaProvider(const juce::String& host, InputStreamFactory streamFactory)
        : Thread("OllamaProviderThread")
        , ollamaHost(host)
        , createStream(std::move(streamFactory)) {}

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

    using juce::Thread::stopThread; // Make stopThread public for testing purposes

    void setModel(const juce::String& name) override;
    juce::String getCurrentModel() const override;
    void fetchAvailableModels(std::function<void(const juce::StringArray& models, bool success)> callback) override;

    void setTestMode(bool testMode) { isTestMode = testMode; }

private:
    juce::String ollamaHost;
    juce::String currentModel = "qwen3-coder-next:latest";
    InputStreamFactory createStream; // Member variable for the stream factory
    bool isTestMode = false;

    struct Request {
        std::vector<Message> conversation;
        AIProvider::CompletionCallback callback;
    };

    juce::CriticalSection queueLock;
    std::vector<Request> pendingRequests;

    void run() override;                     // Declaration for inherited method
    void processRequest(const Request& req); // Declaration for private method

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OllamaProvider)
};

} // namespace gsynth
