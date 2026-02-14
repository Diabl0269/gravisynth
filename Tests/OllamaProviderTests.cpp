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

class OllamaProviderTest : public ::testing::Test {
protected:
    // This is set to an invalid host so it tries to connect and fails,
    // allowing us to test error handling for network requests.
    // In a real scenario, you'd mock the network or use a controllable test server.
    // For now, we expect connection failures.
    gsynth::OllamaProvider provider{"http://invalid-ollama-host:11434"};
    juce::String validOllamaHost = "http://127.0.0.1:11434"; // Assuming local Ollama instance runs here

    void SetUp() override {
        // Ensure provider is stopped before each test
        provider.stopThread(100);
    }

    void TearDown() override { provider.stopThread(100); }
};

TEST_F(OllamaProviderTest, SetAndGetCurrentModel) {
    juce::String modelName = "test-model:latest";
    provider.setModel(modelName);
    ASSERT_EQ(provider.getCurrentModel(), modelName);
}

TEST_F(OllamaProviderTest, FetchAvailableModelsFailsOnInvalidHost) {
    MockCompletionCallback callback;
    provider.fetchAvailableModels(
        [&callback](const juce::StringArray& models, bool success) { callback(models, success); });

    auto result = callback.getResult();
    ASSERT_FALSE(std::get<1>(result));          // Should be unsuccessful
    ASSERT_TRUE(std::get<0>(result).isEmpty()); // Models list should be empty
}

// NOTE: To run the following test, you MUST have an Ollama instance running locally
// with some models pulled. This test is commented out by default for CI/local runs
// without Ollama setup.
/*
TEST_F(OllamaProviderTest, DISABLED_FetchAvailableModelsSuccessOnValidHost)
{
    gsynth::OllamaProvider liveProvider{validOllamaHost};
    MockCompletionCallback callback;
    liveProvider.fetchAvailableModels(callback);

    auto result = callback.getResult();
    ASSERT_TRUE(std::get<1>(result)); // Should be successful
    ASSERT_FALSE(std::get<0>(result).isEmpty()); // Models list should not be empty
    // Further checks could involve parsing the returned StringArray
}

TEST_F(OllamaProviderTest, DISABLED_SendPromptFailsOnInvalidHost)
{
    // provider is already configured with invalid host
    std::vector<gsynth::AIProvider::Message> conversation = {{"user", "hello"}};
    MockCompletionCallback callback;
    provider.sendPrompt(conversation, callback);

    auto result = callback.getResult();
    ASSERT_FALSE(std::get<1>(result)); // Should be unsuccessful
    ASSERT_TRUE(std::get<0>(result).contains("Error: Could not connect to Ollama"));
}

TEST_F(OllamaProviderTest, DISABLED_SendPromptSuccessOnValidHost)
{
    gsynth::OllamaProvider liveProvider{validOllamaHost};
    // Need to set a model that exists on the live Ollama instance
    liveProvider.setModel("qwen3-coder-next:latest");

    std::vector<gsynth::AIProvider::Message> conversation = {{"user", "What is the capital of France?"}};
    MockCompletionCallback callback;
    liveProvider.sendPrompt(conversation, callback);

    auto result = callback.getResult();
    ASSERT_TRUE(std::get<1>(result)); // Should be successful
    ASSERT_FALSE(std::get<0>(result).isEmpty()); // Response should not be empty
    ASSERT_TRUE(std::get<0>(result).contains("Paris") || std::get<0>(result).contains("paris"));
}
*/
