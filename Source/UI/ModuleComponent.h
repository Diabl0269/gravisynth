#pragma once

#include "ScopeComponent.h"
#include <JuceHeader.h>

class GraphEditor; // Forward declaration

class ModuleComponent : public juce::Component, public juce::Timer {
public:
  ModuleComponent(juce::AudioProcessor *module, GraphEditor &owner);
  ~ModuleComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;
  void mouseUp(const juce::MouseEvent &e) override;
  void moved() override;

  juce::AudioProcessor *getModule() const { return module; }

  // Interaction Logic
  struct Port {
    juce::Rectangle<int> area;
    int index;
    bool isInput;
    bool isMidi = false;
  };

  std::optional<Port> getPortForPoint(juce::Point<int> localPoint);
  juce::Point<int> getPortCenter(int index, bool isInput);

private:
  juce::AudioProcessor *module;
  GraphEditor &owner;
  juce::ComponentDragger dragger;

  // Auto-UI
  juce::OwnedArray<juce::Slider> sliders;
  juce::OwnedArray<juce::Label> sliderLabels;
  juce::OwnedArray<juce::ComboBox> comboBoxes;
  juce::OwnedArray<juce::Label> comboLabels;
  juce::OwnedArray<juce::ToggleButton> toggles;

  // Attachments need to be kept alive.
  // We are using raw pointers for parameters currently.
  juce::OwnedArray<juce::SliderParameterAttachment> sliderAttachments;
  juce::OwnedArray<juce::ComboBoxParameterAttachment> comboAttachments;
  juce::OwnedArray<juce::ButtonParameterAttachment> buttonAttachments;

  std::unique_ptr<ScopeComponent> scopeComponent;
  std::unique_ptr<juce::ToggleButton> scopeToggle;

  void createControls();
  void updateLayout();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleComponent)
};
