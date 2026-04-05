#pragma once

#include "AI/AIIntegrationService.h"
#include "AudioEngine.h"
#include "GravisynthUndoManager.h"
#include "PresetManager.h"
#include "UI/AIChatComponent.h"
#include "UI/GraphEditor.h"
#include "UI/ModuleLibraryComponent.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MainComponent
    : public juce::Component
    , public juce::DragAndDropContainer
    , public juce::Timer
    , private gsynth::AIIntegrationService::Listener {
public:
    MainComponent();
    ~MainComponent() override;

    void timerCallback() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Testing Hooks
    bool isAiPanelConfiguredVisible() const { return isAiPanelVisible; }
    void simulateToggleAiPanelClick() {
        if (toggleAiPanelButton.onClick)
            toggleAiPanelButton.onClick();
    }
    void simulateToggleModMatrixClick() {
        if (toggleModMatrixButton.onClick)
            toggleModMatrixButton.onClick();
    }
    GraphEditor& getGraphEditor() { return graphEditor; }

private:
    // AIIntegrationService::Listener
    void aiPatchApplied() override;

    GravisynthUndoManager undoManager;
    AudioEngine audioEngine;
    GraphEditor graphEditor;
    ModuleLibraryComponent moduleLibrary;

    juce::TextButton saveButton;
    juce::TextButton loadButton;
    juce::TextButton settingsButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton toggleAiPanelButton;
    juce::TextButton toggleModMatrixButton;

    std::unique_ptr<juce::FileChooser> fileChooser;

    gsynth::AIIntegrationService aiService;
    gsynth::AIChatComponent aiChatComponent;
    bool isAiPanelVisible = true;

    juce::ApplicationProperties appProperties;
    juce::PropertiesFile::Options propertiesOptions;

    float aiPaneWidth = 300.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
