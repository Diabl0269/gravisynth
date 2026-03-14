#include "../Source/AI/AIIntegrationService.h"
#include "../Source/AI/AIProvider.h"
#include "../Source/UI/AIChatComponent.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

// A simple mock provider so we don't actually hit the network in UI tests
class MockChatProvider : public gsynth::AIProvider {
public:
    juce::String getProviderName() const override { return "MockChatProvider"; }

    void fetchAvailableModels(std::function<void(const juce::StringArray&, bool)> callback) override {
        callback({"MockModel1", "MockModel2"}, true);
    }

    void sendPrompt(const std::vector<gsynth::AIProvider::Message>& conversation, CompletionCallback callback,
                    const juce::var& responseSchema = juce::var()) override {
        // Echo back a mock response synchronously
        callback("Mock response text.", true);
    }

    void setModel(const juce::String& name) override { currentModel = name; }
    juce::String getCurrentModel() const override { return currentModel; }

private:
    juce::String currentModel = "MockModel1";
};

#include "../Source/AudioEngine.h"

class AIChatComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        juce::MessageManager::getInstance();
        engine = std::make_unique<AudioEngine>();
        service = std::make_unique<gsynth::AIIntegrationService>(engine->getGraph());
        service->setProvider(std::make_unique<MockChatProvider>());

        chatComponent = std::make_unique<gsynth::AIChatComponent>(*service);
        chatComponent->setSize(400, 600);
    }

    void TearDown() override {
        chatComponent.reset();
        service.reset();
        engine.reset();
        juce::MessageManager::deleteInstance();
        juce::DeletedAtShutdown::deleteAll();
    }

    std::unique_ptr<AudioEngine> engine;
    std::unique_ptr<gsynth::AIIntegrationService> service;
    std::unique_ptr<gsynth::AIChatComponent> chatComponent;
};

TEST_F(AIChatComponentTest, InitializationAndResizing) { EXPECT_NO_THROW(chatComponent->resized()); }

TEST_F(AIChatComponentTest, SendMessageUpdatesUIAndHistory) {
    // Find the TextEditor and Send Button
    juce::TextEditor* inputField = nullptr;
    juce::TextButton* sendButton = nullptr;

    for (auto* child : chatComponent->getChildren()) {
        if (auto* editor = dynamic_cast<juce::TextEditor*>(child)) {
            inputField = editor;
        } else if (auto* button = dynamic_cast<juce::TextButton*>(child)) {
            sendButton = button;
        }
    }

    ASSERT_NE(inputField, nullptr);
    ASSERT_NE(sendButton, nullptr);

    // Initial history should be empty (excluding system message)
    size_t initialHistorySize = service->getHistory().size(); // Usually 1 system message

    // Simulate user typing a message
    inputField->setText("Create a fat bass synth");

    // Simulate button click synchronously
    if (sendButton->onClick) {
        sendButton->onClick();
    }

    // The message should be added to the history and input cleared
    EXPECT_TRUE(inputField->getText().isEmpty());
    EXPECT_GT(service->getHistory().size(), initialHistorySize);

    // The AI response should now also be in the history because MockChatProvider is synchronous
    EXPECT_GT(service->getHistory().size(), initialHistorySize + 1);
}
