#pragma once

#include <JuceHeader.h>

class ModuleLibraryComponent : public juce::Component {
public:
  ModuleLibraryComponent() {
    moduleNames = {"Oscillator", "Filter",    "LFO",   "ADSR",
                   "VCA",        "Sequencer", "Output"};
  }

  void paint(juce::Graphics &g) override {
    g.fillAll(juce::Colours::darkgrey.darker());
    g.setColour(juce::Colours::white);
    g.setFont(18.0f);

    int y = 20;
    for (const auto &name : moduleNames) {
      g.drawText(name, 20, y, getWidth() - 40, 30,
                 juce::Justification::centredLeft);
      y += 40;
    }
  }

  void mouseDown(const juce::MouseEvent &e) override {
    int index = (e.y - 20) / 40;
    if (index >= 0 && index < moduleNames.size()) {
      juce::Image dragImage(juce::Image::ARGB, 150, 30, true);
      juce::Graphics dg(dragImage);
      dg.setColour(juce::Colours::white);
      dg.setFont(18.0f);
      dg.drawText(moduleNames[index], dragImage.getBounds(),
                  juce::Justification::centred, false);

      juce::DragAndDropContainer::findParentDragContainerFor(this)
          ->startDragging(moduleNames[index], this, dragImage);
    }
  }

private:
  std::vector<juce::String> moduleNames;
};
