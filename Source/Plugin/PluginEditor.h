#pragma once

#include "../AI/AIIntegrationService.h"
#include "../GravisynthUndoManager.h"
#include "../UI/AIChatComponent.h"
#include "../UI/GraphEditor.h"
#include "../UI/ModuleLibraryComponent.h"
#include "PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

class GravisynthPluginEditor
    : public juce::AudioProcessorEditor
    , public juce::DragAndDropContainer
    , public juce::Timer
    , private gsynth::AIIntegrationService::Listener {
public:
    GravisynthPluginEditor(GravisynthPluginProcessor& processor);
    ~GravisynthPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    void aiPatchApplied() override;

    GravisynthPluginProcessor& pluginProcessor;

    GravisynthUndoManager undoManager;
    GraphEditor graphEditor;
    ModuleLibraryComponent moduleLibrary;

    juce::TextButton saveButton;
    juce::TextButton loadButton;
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton toggleAiPanelButton;
    juce::TextButton toggleModMatrixButton;

    std::unique_ptr<juce::FileChooser> fileChooser;

    gsynth::AIIntegrationService aiService;
    gsynth::AIChatComponent aiChatComponent;
    bool isAiPanelVisible = true;
    float aiPaneWidth = 300.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GravisynthPluginEditor)
};
