#include "MainComponent.h"
#include "AI/OllamaProvider.h"

MainComponent::MainComponent()
    : graphEditor(audioEngine)
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
    juce::String savedProviderName =
        appProperties.getUserSettings()->getValue("aiProvider", "Ollama"); // Default to Ollama
    juce::String savedOllamaHost = appProperties.getUserSettings()->getValue("ollamaHost", "http://localhost:11434");

    if (savedProviderName == "Ollama") {
        aiService.setProvider(std::make_unique<gsynth::OllamaProvider>(savedOllamaHost));
    }
    // else if (savedProviderName == "OtherProvider") { ... }

    aiChatComponent.refreshModels();

    aiService.addListener(this);
    setSize(1600, 900);
    addAndMakeVisible(graphEditor);
    addAndMakeVisible(moduleLibrary);
    addAndMakeVisible(aiChatComponent);

    // Buttons
    addAndMakeVisible(saveButton);
    saveButton.setButtonText("Save Preset");
    saveButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
        auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{}) {
                graphEditor.savePreset(file);
            }
        });
    };

    addAndMakeVisible(loadButton);
    loadButton.setButtonText("Load Preset");
    loadButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Preset", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.xml");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc) {
            auto file = fc.getResult();
            if (file != juce::File{}) {
                graphEditor.loadPreset(file);
            }
        });
    };

    addAndMakeVisible(settingsButton);
    settingsButton.setButtonText("Settings");
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
    aiService.removeListener(this);
    audioEngine.shutdown();
}

void MainComponent::aiPatchApplied() {
    juce::MessageManager::callAsync([this]() { graphEditor.updateComponents(); });
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized() {
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(30);

    saveButton.setBounds(header.removeFromLeft(100).reduced(2));
    loadButton.setBounds(header.removeFromLeft(100).reduced(2));
    settingsButton.setBounds(header.removeFromLeft(100).reduced(2));

    aiChatComponent.setBounds(bounds.removeFromRight((int)aiPaneWidth));
    moduleLibrary.setBounds(bounds.removeFromLeft(200));
    graphEditor.setBounds(bounds);
}
