#pragma once

#include <JuceHeader.h>

class ModuleComponent : public juce::Component, public juce::Timer {
public:
  ModuleComponent(juce::AudioProcessor *module);
  ~ModuleComponent() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

  void mouseDown(const juce::MouseEvent &e) override;
  void mouseDrag(const juce::MouseEvent &e) override;

  juce::AudioProcessor *getModule() const { return module; }

private:
  juce::AudioProcessor *module;
  juce::ComponentDragger dragger;

  // Auto-UI
  juce::OwnedArray<juce::Slider> sliders;
  juce::OwnedArray<juce::Label> sliderLabels;
  juce::OwnedArray<juce::ComboBox> comboBoxes;
  juce::OwnedArray<juce::Label> comboLabels;
  juce::OwnedArray<juce::ToggleButton> toggles;

  // Attachments need to be kept alive.
  // We are using raw pointers for params currently, so we'll do simple listener
  // or direct polling. Ideally use ParameterAttachments. Let's simpler
  // approach: Component::Listener or polling? JUCE's
  // GenericAudioProcessorEditor does this well. Let's use std::vector of
  // unique_ptrs to attachments if we had APVTS. Since we used raw
  // AudioParameters, we can use SliderParameterAttachment.

  juce::OwnedArray<juce::SliderParameterAttachment> sliderAttachments;
  juce::OwnedArray<juce::ComboBoxParameterAttachment> comboAttachments;
  juce::OwnedArray<juce::ButtonParameterAttachment> buttonAttachments;

  void createControls();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleComponent)
};
