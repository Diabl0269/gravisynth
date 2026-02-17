#include "../Source/AI/OllamaProvider.h" // Correct path
#include <future>                        // For std::promise/std::future
#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

// Mock AIProvider::CompletionCallback for testing
struct MockCompletionCallback {
    std::promise<std::pair<juce::String, bool>> promise;

    // Operator for fetchAvailableModels (StringArray)
    void operator()(const juce::StringArray& models, bool success) {
        promise.set_value({models.joinIntoString("|"), success});
    }

    // Operator for sendPrompt (String)
    void operator()(const juce::String& response, bool success) { promise.set_value({response, success}); }

    std::pair<juce::String, bool> getResult() { return promise.get_future().get(); }
};

// Mock InputStream for testing network failures/successes
class MockInputStream : public juce::InputStream {
public:
    MockInputStream(const juce::String& content, bool simulateError)
        : buffer(content.toRawUTF8(), content.getNumBytesAsUTF8())
        , currentPosition(0)
        , shouldSimulateError(simulateError) {}

    // Not virtual in juce::InputStream
    bool failedToOpen() const { return shouldSimulateError; }

    juce::int64 getTotalLength() override { return buffer.getSize(); } // Removed const
    juce::int64 getPosition() override { return currentPosition; }     // Removed const
    bool setPosition(juce::int64 newPosition) override {
        if (newPosition >= 0 && newPosition <= getTotalLength()) {
            currentPosition = newPosition;
            return true;
        }
        return false;
    }

    // Must implement this pure virtual method, and it's not const
    bool isExhausted() override { return getPosition() >= getTotalLength(); }

    int read(void* destBuffer, int maxBytesToRead) override {
        if (shouldSimulateError)
            return 0; // Simulate error reading

        auto bytesRemaining = static_cast<int>(getTotalLength() - getPosition());
        auto bytesToRead = juce::jmin(maxBytesToRead, bytesRemaining);

        if (bytesToRead <= 0)
            return 0;

        buffer.copyTo(destBuffer, getPosition(), static_cast<size_t>(bytesToRead));
        currentPosition += bytesToRead;
        return bytesToRead;
    }

private:
    juce::MemoryBlock buffer;
    juce::int64 currentPosition; // Changed to juce::int64
    bool shouldSimulateError;
};

// Mock InputStream that simulates a delay for timeout testing
class SlowInputStream : public juce::InputStream {
public:
    SlowInputStream(int delayMs)
        : delayInMs(delayMs) {}

    bool failedToOpen() const { return false; }         // Always "opens" successfully
    juce::int64 getTotalLength() override { return 1; } // A minimal length to make read work
    juce::int64 getPosition() override { return 0; }
    bool setPosition(juce::int64 newPosition) override {
        juce::ignoreUnused(newPosition);
        return false;
    }
    bool isExhausted() override { return false; }

    int read(void* destBuffer, int maxBytesToRead) override {
        juce::ignoreUnused(destBuffer, maxBytesToRead);
        juce::Thread::sleep(delayInMs); // Simulate a long delay
        return 0;                       // Return 0 to indicate no data read, or error
    }

private:
    int delayInMs;
};

class OllamaProviderTest : public ::testing::Test {
protected:
    // This is set to an invalid host so it tries to connect and fails,
    // allowing us to test error handling for network requests.
    // In a real scenario, you'd mock the network or use a controllable test server.
    // For now, we expect connection failures.
    // gsynth::OllamaProvider provider{"http://invalid-ollama-host:11434"}; // Replaced by mocked provider
    juce::String validOllamaHost = "http://127.0.0.1:11434"; // Assuming local Ollama instance runs here

    // A factory that always returns a stream simulating an an error (returns nullptr)
    static std::unique_ptr<juce::InputStream> createFailingStream(const juce::URL& url,
                                                                  const juce::URL::InputStreamOptions& options) {
        juce::ignoreUnused(url, options);
        return nullptr; // Simulate failed to open stream
    }

    // A factory that returns a stream with a predefined success response
    static std::unique_ptr<juce::InputStream>
    createSuccessfulModelsStream(const juce::URL& url, const juce::URL::InputStreamOptions& options) {
        juce::ignoreUnused(url, options);
        juce::String jsonResponse = R"({"models":[{"name":"mock-model:latest","model":"mock-model:latest"}]})";
        return std::make_unique<MockInputStream>(jsonResponse, false);
    }

    // A factory that returns a stream with a predefined chat response
    static std::unique_ptr<juce::InputStream> createSuccessfulChatStream(const juce::URL& url,
                                                                         const juce::URL::InputStreamOptions& options) {
        juce::ignoreUnused(url, options);
        juce::String jsonResponse =
            R"({"model":"mock-model","message":{"role":"assistant","content":"Mocked AI response."}})";
        return std::make_unique<MockInputStream>(jsonResponse, false);
    }

    // A factory that returns a stream that takes a long time to respond
    static std::unique_ptr<juce::InputStream> createSlowStream(const juce::URL& url,
                                                               const juce::URL::InputStreamOptions& options) {
        juce::ignoreUnused(url);
        // Extract timeout from options. Use a value slightly larger than the actual timeout
        // to ensure the timeout mechanism triggers.
        int timeoutMs = options.getConnectionTimeoutMs();
        int delayMs = timeoutMs + 1000; // Delay for 1 second longer than the timeout
        if (timeoutMs == 0)
            delayMs = 121000; // Default to 2 min + 1 sec if no timeout set

        return std::make_unique<SlowInputStream>(delayMs);
    }

    gsynth::OllamaProvider mockProviderFailingStreams{"http://mock-host:11434", createFailingStream};
    gsynth::OllamaProvider mockProviderSuccessfulModels{"http://mock-host:11434", createSuccessfulModelsStream};
    gsynth::OllamaProvider mockProviderSuccessfulChat{"http://mock-host:11434", createSuccessfulChatStream};

    OllamaProviderTest() {
        mockProviderFailingStreams.setTestMode(true);
        mockProviderSuccessfulModels.setTestMode(true);
        mockProviderSuccessfulChat.setTestMode(true);
    }

    void SetUp() override {
        // Ensure providers are stopped before each test
        mockProviderFailingStreams.stopThread(5000);
        mockProviderSuccessfulModels.stopThread(5000);
        mockProviderSuccessfulChat.stopThread(5000);
    }

    void TearDown() override {
        mockProviderFailingStreams.stopThread(5000);
        mockProviderSuccessfulModels.stopThread(5000);
        mockProviderSuccessfulChat.stopThread(5000);
    }
};

TEST_F(OllamaProviderTest, SetAndGetCurrentModel) {
    juce::String modelName = "test-model:latest";
    mockProviderFailingStreams.setModel(modelName);
    ASSERT_EQ(mockProviderFailingStreams.getCurrentModel(), modelName);
}

TEST_F(OllamaProviderTest, FetchAvailableModelsFailsGracefullyWithMock) {
    MockCompletionCallback callback;
    mockProviderFailingStreams.fetchAvailableModels(
        [&callback](const juce::StringArray& models, bool success) { callback(models, success); });

    auto result = callback.getResult();
    ASSERT_FALSE(std::get<1>(result));          // Should be unsuccessful
    ASSERT_TRUE(std::get<0>(result).isEmpty()); // Models list should be empty
}

TEST_F(OllamaProviderTest, FetchAvailableModelsSuccessWithMock) {
    MockCompletionCallback callback;
    mockProviderSuccessfulModels.fetchAvailableModels(
        [&callback](const juce::StringArray& models, bool success) { callback(models, success); });

    auto result = callback.getResult();
    ASSERT_TRUE(std::get<1>(result));            // Should be successful
    ASSERT_FALSE(std::get<0>(result).isEmpty()); // Models list should not be empty
    ASSERT_TRUE(std::get<0>(result).contains("mock-model:latest"));
}

TEST_F(OllamaProviderTest, SendPromptSuccessWithMock) {
    mockProviderSuccessfulChat.setModel("mock-model:latest");

    std::vector<gsynth::AIProvider::Message> conversation = {{"user", "Hello AI"}};
    MockCompletionCallback callback;
    mockProviderSuccessfulChat.sendPrompt(
        conversation, [&callback](const juce::String& response, bool success) { callback(response, success); });

    auto result = callback.getResult();
    ASSERT_TRUE(std::get<1>(result));            // Should be successful
    ASSERT_FALSE(std::get<0>(result).isEmpty()); // Response should not be empty
    ASSERT_TRUE(std::get<0>(result).contains("Mocked AI response."));
}

TEST_F(OllamaProviderTest, SendPromptTimeoutFails) {
    // Create a provider that uses the slow stream factory
    gsynth::OllamaProvider mockProviderSlowStream{"http://mock-host:11434", createSlowStream};
    mockProviderSlowStream.setTestMode(true);
    mockProviderSlowStream.setModel("mock-model:latest");

    std::vector<gsynth::AIProvider::Message> conversation = {{"user", "Simulate timeout"}};
    MockCompletionCallback callback;
    mockProviderSlowStream.sendPrompt(
        conversation, [&callback](const juce::String& response, bool success) { callback(response, success); });

    auto result = callback.getResult();
    ASSERT_FALSE(std::get<1>(result)); // Should be unsuccessful
    // The response text on timeout is "Error: Could not connect to Ollama at "
    // So we can check for that string or a part of it.
    ASSERT_TRUE(std::get<0>(result).isEmpty()); // The response text should be empty string on timeout
}
