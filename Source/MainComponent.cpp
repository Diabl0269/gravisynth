#include "MainComponent.h"

MainComponent::MainComponent() : graphEditor(audioEngine) {
  setSize(800, 600);
  addAndMakeVisible(graphEditor);
  addAndMakeVisible(moduleLibrary);

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

//==============================================================================
void MainComponent::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized() {
  auto bounds = getLocalBounds();
  moduleLibrary.setBounds(bounds.removeFromLeft(200));
  graphEditor.setBounds(bounds);
}
