#pragma once

#include "../AI/AIIntegrationService.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace gsynth {

/**
 * @class AIChatComponent
 * @brief Enhanced chat interface with collapsible patch cards and loading indicators.
 */
class AIChatComponent
    : public juce::Component
    , private juce::TextEditor::Listener {
public:
    AIChatComponent(AIIntegrationService& service);
    ~AIChatComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void refreshModels();

private:
    class MessageBubble;
    class PatchCard;

    AIIntegrationService& aiService;
    bool isWaitingForResponse = false;

    juce::Viewport viewport;
    juce::Component messageList;
    juce::TextEditor inputField;
    juce::TextButton sendButton;
    juce::ComboBox modelPicker;

    void sendButtonClicked();
    void updateChatDisplay();
    void scrollToBottom();

    struct MessageData {
        juce::String role;
        juce::String text;
        juce::String jsonPatch;
        bool isExpanded = false;
    };
    std::vector<MessageData> messages;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIChatComponent)
};

} // namespace gsynth
