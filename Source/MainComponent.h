#pragma once

#include "AI/AIIntegrationService.h"
#include "AudioEngine.h"
#include "UI/AIChatComponent.h"
#include "UI/GraphEditor.h"
#include "UI/ModuleLibraryComponent.h"
#include <JuceHeader.h>

class MainComponent
    : public juce::Component
    , public juce::DragAndDropContainer
    , private gsynth::AIIntegrationService::Listener {
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // AIIntegrationService::Listener
    void aiPatchApplied() override;

    AudioEngine audioEngine;
    GraphEditor graphEditor;
    ModuleLibraryComponent moduleLibrary;

    juce::TextButton saveButton;
    juce::TextButton loadButton;
    juce::TextButton settingsButton;
    std::unique_ptr<juce::FileChooser> fileChooser;

    gsynth::AIIntegrationService aiService;
    gsynth::AIChatComponent aiChatComponent;

    float aiPaneWidth = 300.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
