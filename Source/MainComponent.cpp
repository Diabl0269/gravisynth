#include "MainComponent.h"

MainComponent::MainComponent() : graphEditor(audioEngine) {
  addAndMakeVisible(graphEditor);
  setSize(1200, 800);

  // Initialize audio engine
  if (juce::RuntimePermissions::isRequired(
          juce::RuntimePermissions::recordAudio) &&
      !juce::RuntimePermissions::isGranted(
          juce::RuntimePermissions::recordAudio)) {
    juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                      [&](bool granted) {
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

MainComponent::~MainComponent() { audioEngine.shutdown(); }

void MainComponent::paint(juce::Graphics &g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized() { graphEditor.setBounds(getLocalBounds()); }
