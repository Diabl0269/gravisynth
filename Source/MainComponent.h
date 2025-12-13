#pragma once

#include "AudioEngine.h"
#include "UI/GraphEditor.h"
#include <JuceHeader.h>

class MainComponent : public juce::Component {
public:
  MainComponent();
  ~MainComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  AudioEngine audioEngine;
  GraphEditor graphEditor;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
