#pragma once

#include "../AudioEngine.h"
#include <JuceHeader.h>

class ModuleComponent;

class GraphEditor : public juce::Component, public juce::Timer {
public:
  GraphEditor(AudioEngine &engine);
  ~GraphEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  void timerCallback() override;
  void mouseDoubleClick(const juce::MouseEvent &) override;
  void updateComponents();

private:
  AudioEngine &audioEngine;
  juce::OwnedArray<ModuleComponent> moduleComponents;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphEditor)
};
