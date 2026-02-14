#include "MainComponent.h"
#include "AI/OllamaProvider.h"

MainComponent::MainComponent()
    : graphEditor(audioEngine)
    , aiService(audioEngine.getGraph())
    , aiChatComponent(aiService) {
    // Initialise AI with Ollama
    aiService.setProvider(std::make_unique<gsynth::OllamaProvider>());
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
    settingsButton.onClick = [this] {
        auto* selector = new juce::AudioDeviceSelectorComponent(audioEngine.getDeviceManager(), 0, 2, // min/max inputs
                                                                0, 2,                                 // min/max outputs
                                                                false, false,                         // midis
                                                                false, false                          // bit depths
        );
        selector->setSize(400, 400);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(selector);
        options.dialogTitle = "Audio Settings";
        options.componentToCentreAround = this;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
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
