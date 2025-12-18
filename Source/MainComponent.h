#pragma once

#include "AudioEngine.h"
#include "UI/GraphEditor.h"
#include "UI/ModuleLibraryComponent.h"
#include <JuceHeader.h>

class MainComponent : public juce::Component,
                      public juce::DragAndDropContainer {
public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  AudioEngine audioEngine;
  GraphEditor graphEditor;
  ModuleLibraryComponent moduleLibrary;

  juce::TextButton saveButton;
  juce::TextButton loadButton;
  std::unique_ptr<juce::FileChooser> fileChooser;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
