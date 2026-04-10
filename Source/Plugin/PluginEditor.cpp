#include "PluginEditor.h"
#include "../PresetManager.h"

GravisynthPluginEditor::GravisynthPluginEditor(GravisynthPluginProcessor& processor)
    : juce::AudioProcessorEditor(processor)
    , pluginProcessor(processor)
    , graphEditor(processor, &undoManager)
    , aiService(processor.getGraph())
    , aiChatComponent(aiService) {
    aiService.addListener(this);
    undoManager.setGraphEditor(&graphEditor);
    setWantsKeyboardFocus(true);
    startTimerHz(10);

    setResizable(true, false);
    setResizeLimits(800, 600, 2400, 1600);
    setSize(1200, 800);

    addAndMakeVisible(graphEditor);
    addAndMakeVisible(moduleLibrary);
    addAndMakeVisible(aiChatComponent);

    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save Preset");
    saveButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{}) {
                graphEditor.savePreset(file);
            }
        });
    };

    addAndMakeVisible(loadButton);
    loadButton.setButtonText("Load Presets");
    loadButton.onClick = [this] {
        juce::PopupMenu menu;
        auto presets = gsynth::PresetManager::getPresetList();
        auto categories = gsynth::PresetManager::getCategories();
        for (const auto& cat : categories) {
            juce::PopupMenu subMenu;
            for (int i = 0; i < presets.size(); ++i) {
                if (presets[i].category == cat)
                    subMenu.addItem(i + 1, presets[i].name);
            }
            menu.addSubMenu(cat, subMenu);
        }
        menu.addSeparator();
        menu.addItem(1000, "Load from file...");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&loadButton), [this](int result) {
            if (result == 1000) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.json");
                auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
                fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File{}) {
                        graphEditor.loadPreset(file);
                    }
                });
            } else if (result > 0) {
                if (gsynth::PresetManager::loadPreset(result - 1, pluginProcessor.getGraph())) {
                    graphEditor.updateComponents();
                }
            }
        });
    };

    addAndMakeVisible(undoButton);
    undoButton.setButtonText("Undo");
    undoButton.setEnabled(false);
    undoButton.onClick = [this] {
        if (undoManager.canUndo())
            undoManager.undo();
        undoButton.setEnabled(undoManager.canUndo());
        redoButton.setEnabled(undoManager.canRedo());
    };

    addAndMakeVisible(redoButton);
    redoButton.setButtonText("Redo");
    redoButton.setEnabled(false);
    redoButton.onClick = [this] {
        if (undoManager.canRedo())
            undoManager.redo();
        undoButton.setEnabled(undoManager.canUndo());
        redoButton.setEnabled(undoManager.canRedo());
    };

    addAndMakeVisible(toggleAiPanelButton);
    toggleAiPanelButton.setButtonText("Hide AI");
    toggleAiPanelButton.onClick = [this] {
        isAiPanelVisible = !isAiPanelVisible;
        aiChatComponent.setVisible(isAiPanelVisible);
        toggleAiPanelButton.setButtonText(isAiPanelVisible ? "Hide AI" : "Show AI");
        resized();
    };

    addAndMakeVisible(toggleModMatrixButton);
    toggleModMatrixButton.setButtonText("Hide Matrix");
    toggleModMatrixButton.onClick = [this] {
        graphEditor.toggleModMatrixVisibility();
        toggleModMatrixButton.setButtonText(graphEditor.isModMatrixVisible() ? "Hide Matrix" : "Show Matrix");
        resized();
    };

    graphEditor.updateComponents();
}

GravisynthPluginEditor::~GravisynthPluginEditor() {
    stopTimer();
    aiService.removeListener(this);
    graphEditor.detachAllModuleComponents();
}

void GravisynthPluginEditor::timerCallback() {
    undoButton.setEnabled(undoManager.canUndo());
    redoButton.setEnabled(undoManager.canRedo());
}

void GravisynthPluginEditor::aiPatchApplied() {
    juce::MessageManager::callAsync([this]() { graphEditor.updateComponents(); });
}

void GravisynthPluginEditor::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

bool GravisynthPluginEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) {
        if (undoManager.canRedo())
            undoManager.redo();
        return true;
    }
    if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0)) {
        if (undoManager.canUndo())
            undoManager.undo();
        return true;
    }
    return false;
}

void GravisynthPluginEditor::resized() {
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(30);

    saveButton.setBounds(header.removeFromLeft(100).reduced(2));
    loadButton.setBounds(header.removeFromLeft(120).reduced(2));
    undoButton.setBounds(header.removeFromLeft(80).reduced(2));
    redoButton.setBounds(header.removeFromLeft(80).reduced(2));

    toggleAiPanelButton.setBounds(header.removeFromRight(100).reduced(2));
    toggleModMatrixButton.setBounds(header.removeFromRight(100).reduced(2));

    if (isAiPanelVisible) {
        aiChatComponent.setBounds(bounds.removeFromRight((int)aiPaneWidth));
    }

    moduleLibrary.setBounds(bounds.removeFromLeft(200));
    graphEditor.setBounds(bounds);
}
