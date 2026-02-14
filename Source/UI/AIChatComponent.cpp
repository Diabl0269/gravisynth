#include "AIChatComponent.h"

namespace gsynth {

//==============================================================================
class AIChatComponent::PatchCard : public juce::Component {
public:
    PatchCard(const juce::String& json, std::function<void()> applyCallback)
        : patchJson(json)
        , onApply(applyCallback) {

        addAndMakeVisible(headerLabel);
        headerLabel.setText("Suggested Patch", juce::dontSendNotification);
        headerLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);

        addAndMakeVisible(expandButton);
        expandButton.setButtonText("Expand JSON");
        expandButton.setToggleable(true);
        expandButton.onClick = [this]() {
            isExpanded = !isExpanded;
            expandButton.setButtonText(isExpanded ? "Collapse" : "Expand JSON");
            if (auto* parent = getParentComponent())
                parent->resized();
        };

        addAndMakeVisible(applyButton);
        applyButton.setButtonText("Apply Patch");
        applyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
        applyButton.onClick = onApply;

        addAndMakeVisible(jsonDisplay);
        jsonDisplay.setMultiLine(true);
        jsonDisplay.setReadOnly(true);
        jsonDisplay.setText(patchJson);
        jsonDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.3f));
        jsonDisplay.setVisible(false);
    }

    void resized() override {
        auto b = getLocalBounds().reduced(5);
        auto header = b.removeFromTop(25);
        headerLabel.setBounds(header.removeFromLeft(120));

        auto buttons = header.removeFromRight(160);
        applyButton.setBounds(buttons.removeFromRight(80).reduced(2));
        expandButton.setBounds(buttons.removeFromRight(80).reduced(2));

        if (isExpanded) {
            b.removeFromTop(5);
            jsonDisplay.setVisible(true);
            jsonDisplay.setBounds(b);
        } else {
            jsonDisplay.setVisible(false);
        }
    }

    int getRequiredHeight() const { return isExpanded ? 200 : 35; }

private:
    juce::String patchJson;
    std::function<void()> onApply;
    bool isExpanded = false;

    juce::Label headerLabel;
    juce::TextButton expandButton;
    juce::TextButton applyButton;
    juce::TextEditor jsonDisplay;
};

//==============================================================================
class AIChatComponent::MessageBubble : public juce::Component {
public:
    MessageBubble(const MessageData& data, std::function<void(const juce::String&)> applyPatch) {
        role = data.role;
        text = data.text;

        addAndMakeVisible(textLabel);
        textLabel.setText(text, juce::dontSendNotification);
        textLabel.setMinimumHorizontalScale(1.0f);
        textLabel.setJustificationType(juce::Justification::topLeft);

        if (data.jsonPatch.isNotEmpty()) {
            patchCard = std::make_unique<PatchCard>(data.jsonPatch,
                                                    [applyPatch, json = data.jsonPatch]() { applyPatch(json); });
            addAndMakeVisible(*patchCard);
        }
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().reduced(2).toFloat();
        bool isUser = (role == "user");

        juce::Colour baseColour = isUser ? juce::Colours::blue : juce::Colours::darkgrey.brighter(0.2f);
        juce::ColourGradient grad(baseColour.withAlpha(0.3f), b.getX(), b.getY(), baseColour.withAlpha(0.1f),
                                  b.getRight(), b.getBottom(), false);

        g.setGradientFill(grad);
        g.fillRoundedRectangle(b, 10.0f);

        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRoundedRectangle(b, 10.0f, 1.0f);

        // Role indicator
        g.setColour(isUser ? juce::Colours::lightblue : juce::Colours::grey);
        g.setFont(juce::Font(10.0f, juce::Font::italic));
        g.drawText(isUser ? "YOU" : "AI", b.removeFromTop(12).reduced(5, 0), juce::Justification::left);
    }

    void resized() override {
        auto b = getLocalBounds().reduced(10);

        if (patchCard) {
            patchCard->setBounds(b.removeFromBottom(patchCard->getRequiredHeight()));
            b.removeFromBottom(5);
        }

        textLabel.setBounds(b);
    }

    int getRequiredHeight(int width) {
        int contentWidth = width - 20; // Margin
        juce::Font font = textLabel.getFont();

        juce::GlyphArrangement ga;
        ga.addJustifiedText(font, text, 0.0f, 0.0f, (float)contentWidth, juce::Justification::left);

        int textHeight = (int)ga.getBoundingBox(0, -1, true).getHeight();
        int height = textHeight + 25; // Base height for text + role label

        if (patchCard) {
            height += 10 + patchCard->getRequiredHeight();
        }

        return juce::jmax(40, height);
    }

private:
    juce::String role;
    juce::String text;
    juce::Label textLabel;
    std::unique_ptr<PatchCard> patchCard;
};

//==============================================================================
AIChatComponent::AIChatComponent(AIIntegrationService& service)
    : aiService(service) {

    addAndMakeVisible(viewport);
    viewport.setScrollBarsShown(true, false);
    viewport.setViewedComponent(&messageList);

    addAndMakeVisible(inputField);
    inputField.setReturnKeyStartsNewLine(false);
    inputField.onReturnKey = [this]() { sendButtonClicked(); };
    inputField.addListener(this);
    inputField.setTextToShowWhenEmpty("Ask AI to create or modify a patch...", juce::Colours::grey);

    addAndMakeVisible(sendButton);
    sendButton.setButtonText("Send");
    sendButton.onClick = [this]() { sendButtonClicked(); };

#ifdef NDEBUG
    juce::Logger::writeToLog("AIChatComponent initialized (Release)");
#else
    juce::Logger::writeToLog("AIChatComponent initialized (Debug)");
#endif

    addAndMakeVisible(modelPicker);
    modelPicker.onChange = [this]() { aiService.setModel(modelPicker.getText()); };

    refreshModels();

    // Populate history
    for (const auto& msg : aiService.getHistory()) {
        if (msg.role == "system")
            continue;

        juce::String json;
        juce::String cleanText = msg.content;
        int start = msg.content.indexOf("```json");
        if (start != -1) {
            int end = msg.content.indexOf(start + 7, "```");
            if (end != -1) {
                json = msg.content.substring(start + 7, end).trim();
                cleanText = msg.content.substring(0, start) + msg.content.substring(end + 3);
            }
        }
        messages.push_back({msg.role, cleanText.trim(), json});
    }

    updateChatDisplay();
}

AIChatComponent::~AIChatComponent() {}

void AIChatComponent::resized() {
    auto b = getLocalBounds().reduced(10);

    auto bottomArea = b.removeFromBottom(70); // Increased height for both rows

    // Bottom row: Input + Send
    auto inputRow = bottomArea.removeFromBottom(40);
    sendButton.setBounds(inputRow.removeFromRight(60));
    inputRow.removeFromRight(10);
    inputField.setBounds(inputRow);

    // Middle row (above input): Model Picker
    bottomArea.removeFromBottom(5);
    auto modelRow = bottomArea.removeFromBottom(25);
    modelPicker.setBounds(modelRow.removeFromLeft(200));

    b.removeFromBottom(10);
    viewport.setBounds(b);

    // Layout message bubbles and loader
    int y = 0;
    int width = viewport.getMaximumVisibleWidth();
    for (auto* child : messageList.getChildren()) {
        int h = 0;
        if (auto* bubble = dynamic_cast<MessageBubble*>(child)) {
            h = bubble->getRequiredHeight(width);
        } else if (dynamic_cast<juce::Label*>(child)) {
            h = 24;
        }

        if (h > 0) {
            child->setBounds(0, y, width, h);
            y += h + 10;
        }
    }
    messageList.setSize(width, juce::jmax(viewport.getHeight(), y));
}

void AIChatComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colours::darkgrey.darker(0.5f)); }

void AIChatComponent::sendButtonClicked() {
    auto text = inputField.getText().trim();
    if (text.isEmpty())
        return;

    inputField.clear();

    // Add user message to local state immediately
    messages.push_back({"user", text, ""});
    isWaitingForResponse = true;
    updateChatDisplay();

    sendButton.setEnabled(false);
    inputField.setReadOnly(true);

    aiService.sendMessage(text, [this](const juce::String& response, bool success) {
        juce::MessageManager::callAsync([this, response, success]() {
            isWaitingForResponse = false;
            sendButton.setEnabled(true);
            inputField.setReadOnly(false);

            if (success) {
                // Parse for JSON
                juce::String json;
                juce::String cleanText = response;
                int start = response.indexOf("```json");
                if (start != -1) {
                    int end = response.indexOf(start + 7, "```");
                    if (end != -1) {
                        json = response.substring(start + 7, end).trim();
                        cleanText = response.substring(0, start) + response.substring(end + 3);
                    }
                }
                messages.push_back({"assistant", cleanText.trim(), json});
            } else {
                messages.push_back({"assistant", "Error: Failed to get response from AI.", ""});
            }

            updateChatDisplay();
            inputField.grabKeyboardFocus();
        });
    });
}

void AIChatComponent::updateChatDisplay() {
    messageList.deleteAllChildren();

    for (const auto& data : messages) {
        auto* bubble = new MessageBubble(data, [this](const juce::String& json) { aiService.applyPatch(json); });
        messageList.addAndMakeVisible(bubble);
    }

    if (isWaitingForResponse) {
        auto* loading = new juce::Label();
        messageList.addAndMakeVisible(loading);
        loading->setText("AI is thinking...", juce::dontSendNotification);
        loading->setColour(juce::Label::textColourId, juce::Colours::grey);
    }

    resized();
    scrollToBottom();
}

void AIChatComponent::scrollToBottom() { viewport.setViewPosition(0, messageList.getHeight()); }

void AIChatComponent::refreshModels() {
    modelPicker.addItem("Loading models...", 1);
    modelPicker.setSelectedId(1, juce::dontSendNotification);
    modelPicker.setEnabled(false);

    aiService.fetchAvailableModels([this](const juce::StringArray& models, bool success) {
        modelPicker.clear(juce::dontSendNotification);
        modelPicker.setEnabled(true);

        if (success && !models.isEmpty()) {
            for (int i = 0; i < models.size(); ++i) {
                modelPicker.addItem(models[i], i + 1);
            }

            // Try to select the current model
            juce::String current = aiService.getCurrentModel();
            int index = models.indexOf(current);
            if (index != -1)
                modelPicker.setSelectedId(index + 1, juce::dontSendNotification);
            else
                modelPicker.setSelectedId(1, juce::dontSendNotification);
        } else {
            modelPicker.addItem("Error fetching models", 1);
            modelPicker.setSelectedId(1, juce::dontSendNotification);
        }
    });
}

} // namespace gsynth
