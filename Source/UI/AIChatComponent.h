#pragma once

#include "../AI/AIIntegrationService.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace gsynth {

/**
 * @class AIChatComponent
 * @brief Simple chat interface for interacting with the AI.
 */
class AIChatComponent
    : public juce::Component
    , private juce::TextEditor::Listener {
public:
    AIChatComponent(AIIntegrationService& service)
        : aiService(service) {

        addAndMakeVisible(chatDisplay);
        chatDisplay.setMultiLine(true);
        chatDisplay.setReadOnly(true);
        chatDisplay.setCaretVisible(false);
        chatDisplay.setScrollBarThickness(10);
        chatDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.2f));

        addAndMakeVisible(inputField);
        inputField.setReturnKeyStartsNewLine(false);
        inputField.onReturnKey = [this]() { sendButtonClicked(); };
        inputField.addListener(this);
        inputField.setTextToShowWhenEmpty("Ask AI to create or modify a patch...", juce::Colours::grey);

        addAndMakeVisible(sendButton);
        sendButton.setButtonText("Send");
        sendButton.onClick = [this]() { sendButtonClicked(); };

        updateChatDisplay();
    }

    void resized() override {
        auto b = getLocalBounds().reduced(10);
        auto bottomArea = b.removeFromBottom(40);

        sendButton.setBounds(bottomArea.removeFromRight(60));
        bottomArea.removeFromRight(10);
        inputField.setBounds(bottomArea);

        b.removeFromBottom(10);
        chatDisplay.setBounds(b);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::darkgrey.darker(0.5f));
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRect(getLocalBounds(), 1);

        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        // g.drawText("AI Assistant", getLocalBounds().removeFromTop(30), juce::Justification::centred);
    }

private:
    AIIntegrationService& aiService;

    juce::TextEditor chatDisplay;
    juce::TextEditor inputField;
    juce::TextButton sendButton;

    void sendButtonClicked() {
        auto text = inputField.getText().trim();
        if (text.isEmpty())
            return;

        inputField.clear();
        updateChatDisplay(); // Show user message immediately

        sendButton.setEnabled(false);
        inputField.setReadOnly(true);

        aiService.sendMessage(text, [this](const juce::String& response, bool success) {
            juce::MessageManager::callAsync([this, success]() {
                sendButton.setEnabled(true);
                inputField.setReadOnly(false);
                updateChatDisplay();
                inputField.grabKeyboardFocus();
            });
        });
    }

    void updateChatDisplay() {
        juce::String combined;
        for (const auto& msg : aiService.getHistory()) {
            if (msg.role == "system")
                continue;

            combined += (msg.role == "user" ? "You: " : "AI: ");
            combined += msg.content + "\n\n";
        }
        chatDisplay.setText(combined, false);
        chatDisplay.moveCaretToEnd();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIChatComponent)
};

} // namespace gsynth
