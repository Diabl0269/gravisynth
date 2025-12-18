#pragma once

#include "../AudioEngine.h"
#include <JuceHeader.h>

class ModuleComponent;

class GraphEditor : public juce::Component,
                    public juce::Timer,
                    public juce::DragAndDropTarget {
public:
  GraphEditor(AudioEngine &engine);
  ~GraphEditor() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

  void timerCallback() override;
  void updateComponents();

  // Interactions
  void beginConnectionDrag(ModuleComponent *sourceModule, int channelIndex,
                           bool isInput, bool isMidi,
                           juce::Point<int> screenPos);
  void dragConnection(juce::Point<int> screenPos);
  void endConnectionDrag(juce::Point<int> screenPos);
  void disconnectPort(ModuleComponent *module, int portIndex, bool isInput,
                      bool isMidi);
  void deleteModule(ModuleComponent *module);
  void updateModulePosition(ModuleComponent *module);

  // Preset Management
  void savePreset(juce::File file);
  void loadPreset(juce::File file);

  // DragAndDropTarget overrides
  bool
  isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
  void itemDropped(const SourceDetails &dragSourceDetails) override;

private:
  AudioEngine &audioEngine;
  juce::OwnedArray<ModuleComponent> moduleComponents;

  // Drag State
  bool isDraggingConnection = false;
  ModuleComponent *dragSourceModule = nullptr;
  int dragSourceChannel = 0;
  bool dragSourceIsInput = false;
  bool dragSourceIsMidi = false;
  juce::Point<int> dragCurrentPos;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphEditor)
};
