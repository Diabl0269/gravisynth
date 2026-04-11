#include "MainComponent.h"
#include "AI/OllamaProvider.h"

MainComponent::MainComponent(std::unique_ptr<gsynth::AIProvider> provider)
    : graphEditor(audioEngine, &undoManager)
    , aiService(audioEngine.getGraph())
    , aiChatComponent(aiService) {
    // Setup ApplicationProperties
    propertiesOptions.applicationName = "Gravisynth";
    propertiesOptions.folderName = "Gravisynth";
    propertiesOptions.filenameSuffix = "settings";
    propertiesOptions.osxLibrarySubFolder = "Application Support";
    propertiesOptions.storageFormat = juce::PropertiesFile::storeAsXML;
    appProperties.setStorageParameters(propertiesOptions);

    // Load AI provider preference
    juce::String savedProviderName = appProperties.getUserSettings()->getValue("aiProvider", "Ollama");
    juce::String savedOllamaHost = appProperties.getUserSettings()->getValue("ollamaHost", "http://localhost:11434");

    if (provider) {
        aiService.setProvider(std::move(provider));
    } else if (savedProviderName == "Ollama") {
        aiService.setProvider(std::make_unique<gsynth::OllamaProvider>(savedOllamaHost));
    }

    aiChatComponent.refreshModels();

    aiService.addListener(this);
    setSize(1600, 900);
    undoManager.setGraphEditor(&graphEditor);
    setWantsKeyboardFocus(true);
    startTimerHz(10);
    addAndMakeVisible(graphEditor);
    addAndMakeVisible(moduleLibrary);
    addAndMakeVisible(aiChatComponent);

    // Buttons
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save Preset");
    saveButton.setComponentID("saveButton");
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
    loadButton.setComponentID("loadButton");
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
                if (gsynth::PresetManager::loadPreset(result - 1, audioEngine.getGraph())) {
                    graphEditor.updateComponents();
                }
            }
        });
    };

    addAndMakeVisible(undoButton);
    undoButton.setButtonText("Undo");
    undoButton.setComponentID("undoButton");
    undoButton.setEnabled(false);
    undoButton.onClick = [this] {
        if (undoManager.canUndo())
            undoManager.undo();
        undoButton.setEnabled(undoManager.canUndo());
        redoButton.setEnabled(undoManager.canRedo());
    };

    addAndMakeVisible(redoButton);
    redoButton.setButtonText("Redo");
    redoButton.setComponentID("redoButton");
    redoButton.setEnabled(false);
    redoButton.onClick = [this] {
        if (undoManager.canRedo())
            undoManager.redo();
        undoButton.setEnabled(undoManager.canUndo());
        redoButton.setEnabled(undoManager.canRedo());
    };

    addAndMakeVisible(toggleAiPanelButton);
    toggleAiPanelButton.setButtonText("Hide AI");
    toggleAiPanelButton.setComponentID("toggleAiPanel");
    toggleAiPanelButton.onClick = [this] {
        isAiPanelVisible = !isAiPanelVisible;
        aiChatComponent.setVisible(isAiPanelVisible);
        toggleAiPanelButton.setButtonText(isAiPanelVisible ? "Hide AI" : "Show AI");
        resized();
    };

    addAndMakeVisible(toggleModMatrixButton);
    toggleModMatrixButton.setButtonText("Hide Matrix");
    toggleModMatrixButton.setComponentID("toggleModMatrix");
    toggleModMatrixButton.onClick = [this] {
        graphEditor.toggleModMatrixVisibility();
        toggleModMatrixButton.setButtonText(graphEditor.isModMatrixVisible() ? "Hide Matrix" : "Show Matrix");
        resized();
    };

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
    settingsButton.setComponentID("settingsButton");
    settingsButton.onClick = [this, savedProviderName, savedOllamaHost]() mutable { // Capture by value
        // Audio Settings Dialog
        auto* audioSelector =
            new juce::AudioDeviceSelectorComponent(audioEngine.getDeviceManager(), 0, 2, // min/max inputs
                                                   0, 2,                                 // min/max outputs
                                                   false, false,                         // midis
                                                   false, false                          // bit depths
            );
        audioSelector->setSize(400, 400);

        juce::DialogWindow::LaunchOptions audioOptions;
        audioOptions.content.setOwned(audioSelector);
        audioOptions.dialogTitle = "Audio Settings";
        audioOptions.componentToCentreAround = this;
        audioOptions.useNativeTitleBar = true;
        audioOptions.resizable = false;
        audioOptions.launchAsync();

        // AI Settings Dialog
        // Create a custom component for AI settings
        class AISettingsComponent : public juce::Component {
        public:
            AISettingsComponent(juce::ApplicationProperties& props, gsynth::AIIntegrationService& aiServ,
                                gsynth::AIChatComponent& aiChatComp)
                : appProperties(props)
                , aiService(aiServ)
                , aiChatComponent(aiChatComp) {
                addAndMakeVisible(providerLabel);
                providerLabel.setText("AI Provider:", juce::dontSendNotification);
                providerLabel.setBounds(10, 10, 100, 25);

                addAndMakeVisible(providerCombo);
                providerCombo.addItem("Ollama", 1);
                providerCombo.setSelectedId(
                    appProperties.getUserSettings()->getValue("aiProvider", "Ollama") == "Ollama" ? 1 : 0,
                    juce::dontSendNotification);
                providerCombo.setBounds(120, 10, 200, 25);
                providerCombo.onChange = [this] { updateSettings(); };

                addAndMakeVisible(hostLabel);
                hostLabel.setText("Ollama Host:", juce::dontSendNotification);
                hostLabel.setBounds(10, 40, 100, 25);

                addAndMakeVisible(hostEditor);
                hostEditor.setText(appProperties.getUserSettings()->getValue("ollamaHost", "http://localhost:11434"));
                hostEditor.setBounds(120, 40, 250, 25);
                hostEditor.onReturnKey = [this] { updateSettings(); };
                hostEditor.onFocusLost = [this] { updateSettings(); };
            }

            void updateSettings() {
                juce::String selectedProvider = providerCombo.getText();
                juce::String newOllamaHost = hostEditor.getText();

                appProperties.getUserSettings()->setValue("aiProvider", selectedProvider);
                appProperties.getUserSettings()->setValue("ollamaHost", newOllamaHost);
                appProperties.saveIfNeeded();

                // Re-initialize AI service with new provider/host
                if (selectedProvider == "Ollama") {
                    aiService.setProvider(std::make_unique<gsynth::OllamaProvider>(newOllamaHost));
                }
                aiChatComponent.refreshModels();
            }

        private:
            juce::ApplicationProperties& appProperties;
            gsynth::AIIntegrationService& aiService;
            gsynth::AIChatComponent& aiChatComponent;

            juce::Label providerLabel;
            juce::ComboBox providerCombo;
            juce::Label hostLabel;
            juce::TextEditor hostEditor;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AISettingsComponent)
        };

        auto* aiSettingsComp = new AISettingsComponent(appProperties, aiService, aiChatComponent);
        aiSettingsComp->setSize(400, 200);

        juce::DialogWindow::LaunchOptions aiOptions;
        aiOptions.content.setOwned(aiSettingsComp);
        aiOptions.dialogTitle = "AI Settings";
        aiOptions.componentToCentreAround = this;
        aiOptions.useNativeTitleBar = true;
        aiOptions.resizable = false;
        aiOptions.launchAsync();
    };

    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio) &&
        !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio)) {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio, [&](bool granted) {
            if (granted) {
                audioEngine.initialise();
                graphEditor.updateComponents();
            }
        });
    } else {
        audioEngine.initialise();
        graphEditor.updateComponents();
    }
}

MainComponent::~MainComponent() {
    stopTimer();
    aiService.removeListener(this);
    graphEditor.detachAllModuleComponents();
    audioEngine.shutdown();
}

void MainComponent::timerCallback() {
    undoButton.setEnabled(undoManager.canUndo());
    redoButton.setEnabled(undoManager.canRedo());
}

void MainComponent::aiPatchApplied() {
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]() {
        if (auto* self = safeThis.getComponent())
            self->graphEditor.updateComponents();
    });
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

bool MainComponent::keyPressed(const juce::KeyPress& key) {
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

void MainComponent::resized() {
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(30);

    saveButton.setBounds(header.removeFromLeft(100).reduced(2));
    loadButton.setBounds(header.removeFromLeft(120).reduced(2));
    settingsButton.setBounds(header.removeFromLeft(100).reduced(2));
    undoButton.setBounds(header.removeFromLeft(80).reduced(2));
    redoButton.setBounds(header.removeFromLeft(80).reduced(2));

    // Position toggle buttons on the right side of the header
    toggleAiPanelButton.setBounds(header.removeFromRight(100).reduced(2));
    toggleModMatrixButton.setBounds(header.removeFromRight(100).reduced(2));

    if (isAiPanelVisible) {
        aiChatComponent.setBounds(bounds.removeFromRight((int)aiPaneWidth));
    }

    moduleLibrary.setBounds(bounds.removeFromLeft(200));
    graphEditor.setBounds(bounds);
}
