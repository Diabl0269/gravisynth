#include "ModuleComponent.h"
#include "../Modules/SequencerModule.h"

ModuleComponent::ModuleComponent(juce::AudioProcessor *m) : module(m) {
  createControls();
  startTimerHz(30); // 30 FPS for step visualization
}

ModuleComponent::~ModuleComponent() { stopTimer(); }
void ModuleComponent::timerCallback() { repaint(); }

void ModuleComponent::createControls() {
  const auto &params = module->getParameters();

  for (auto *param : params) {
    if (auto *choiceParam = dynamic_cast<juce::AudioParameterChoice *>(param)) {
      auto *combo = comboBoxes.add(new juce::ComboBox());
      combo->addItemList(choiceParam->choices, 1);
      addAndMakeVisible(combo);

      auto *attach = comboAttachments.add(
          new juce::ComboBoxParameterAttachment(*choiceParam, *combo));

      auto *label = comboLabels.add(
          new juce::Label(param->getName(100), param->getName(100)));
      addAndMakeVisible(label);
    } else if (auto *floatParam =
                   dynamic_cast<juce::AudioParameterFloat *>(param)) {
      auto *slider = sliders.add(new juce::Slider());
      slider->setComponentID(param->getName(100)); // ID for lookup
      if (module->getName().contains("ADSR") ||
          module->getName().contains("Env")) {
        slider->setSliderStyle(juce::Slider::LinearVertical);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
      } else {
        slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
      }
      addAndMakeVisible(slider);

      auto *attach = sliderAttachments.add(
          new juce::SliderParameterAttachment(*floatParam, *slider));

      auto *label = sliderLabels.add(
          new juce::Label(param->getName(100), param->getName(100)));
      label->setJustificationType(juce::Justification::centred);
      addAndMakeVisible(label);
    } else if (auto *intParam =
                   dynamic_cast<juce::AudioParameterInt *>(param)) {
      auto *slider = sliders.add(new juce::Slider());
      slider->setComponentID(param->getName(100)); // ID for lookup
      slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
      slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
      // slider->setRange(intParam->getRange().start,
      // intParam->getRange().end, 1.0); // Attachment handles range
      addAndMakeVisible(slider);

      auto *attach = sliderAttachments.add(
          new juce::SliderParameterAttachment(*intParam, *slider));

      auto *label = sliderLabels.add(
          new juce::Label(param->getName(100), param->getName(100)));
      label->setJustificationType(juce::Justification::centred);
      addAndMakeVisible(label);
    } else if (auto *boolParam =
                   dynamic_cast<juce::AudioParameterBool *>(param)) {
      auto *toggle =
          toggles.add(new juce::ToggleButton(boolParam->getName(100)));
      toggle->setComponentID(boolParam->getName(100)); // ID for Lookup
      addAndMakeVisible(toggle);

      auto *attach = buttonAttachments.add(
          new juce::ButtonParameterAttachment(*boolParam, *toggle));
    }
  }

  // Auto-resize
  if (module->getName() == "Sequencer") {
    setSize(510, 380); // 8 cols * 60 + margins, 3 rows
    return;
  }

  int contentHeight = 40; // Header
  contentHeight += comboBoxes.size() * 50;
  contentHeight += toggles.size() * 30;

  // Grid for sliders (2 columns)
  int numSliders = sliders.size();
  int rows = (numSliders + 1) / 2;
  contentHeight += rows * 80;

  setSize(200, std::max(100, contentHeight + 20));
}

void ModuleComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::lightgrey);

  // Highlight Active Step (Sequencer only)
  if (module->getName() == "Sequencer") {
    if (auto *seq = dynamic_cast<SequencerModule *>(module)) {
      int activeStep = seq->currentActiveStep.load();
      // Coordinates match resized()
      int startX = 10;
      int stepWidth = 60;
      int x = startX + activeStep * stepWidth;

      g.setColour(juce::Colours::yellow.withAlpha(0.3f));
      g.fillRect(x, 110, stepWidth - 5, 220); // Cover Gate+Pitch+F.Env area
    }
  }

  g.setColour(juce::Colours::black);
  g.drawRect(getLocalBounds(), 2);

  // Header
  g.setColour(juce::Colours::darkgrey);
  g.fillRect(0, 0, getWidth(), 24);
  g.setColour(juce::Colours::white);
  g.drawText(module->getName(), 0, 0, getWidth(), 24,
             juce::Justification::centred, true);
}

void ModuleComponent::resized() {
  if (module->getName() == "Sequencer") {
    // --- Sequencer Specific Layout ---
    int x = 10;
    int y = 30;

    // Top Row: Run and BPM
    for (auto *toggle : toggles) {
      if (toggle->getComponentID().equalsIgnoreCase("run")) {
        toggle->setBounds(x, y, 60, 24);
        x += 70;
      }
    }

    for (int i = 0; i < sliders.size(); ++i) {
      if (sliders[i]->getComponentID().equalsIgnoreCase("bpm")) {
        sliderLabels[i]->setBounds(x, y, 60, 20);
        sliders[i]->setBounds(x, y + 20, 60, 50);
        x += 70;
      }
    }

    // Steps Row
    // Steps Row
    int startX = 10;
    int startY = 110;
    int stepWidth = 60;

    for (int step = 1; step <= 8; ++step) {
      int colX = startX + (step - 1) * stepWidth;

      // Gate
      juce::String gateId = "Gate " + juce::String(step);
      for (int i = 0; i < sliders.size(); ++i) {
        if (sliders[i]->getComponentID().equalsIgnoreCase(gateId)) {
          sliderLabels[i]->setBounds(colX, startY, stepWidth - 5, 20);
          sliders[i]->setBounds(colX, startY + 20, stepWidth - 5, 50);
        }
      }

      // Pitch
      juce::String pitchId = "Pitch " + juce::String(step);
      for (int i = 0; i < sliders.size(); ++i) {
        if (sliders[i]->getComponentID().equalsIgnoreCase(pitchId)) {
          sliderLabels[i]->setBounds(colX, startY + 80, stepWidth - 5, 20);
          sliders[i]->setBounds(colX, startY + 100, stepWidth - 5, 50);
        }
      }

      // Filter Env
      juce::String fEnvId = "F.Env " + juce::String(step);
      for (int i = 0; i < sliders.size(); ++i) {
        if (sliders[i]->getComponentID().equalsIgnoreCase(fEnvId)) {
          sliderLabels[i]->setBounds(colX, startY + 160, stepWidth - 5, 20);
          sliders[i]->setBounds(colX, startY + 180, stepWidth - 5, 50);
        }
      }
    }

    return;
  }

  // --- ADSR Layout ---
  if (module->getName().contains("ADSR") || module->getName().contains("Env")) {
    setSize(220, 180); // Fixed size for ADSR
    int y = 30;
    int sliderWidth = 50;
    int sliderHeight = 120;

    // We expect 4 sliders: A, D, S, R
    for (int i = 0; i < sliders.size(); ++i) {
      int x = 10 + i * sliderWidth;
      sliderLabels[i]->setBounds(x, y, sliderWidth, 20);
      sliders[i]->setBounds(x, y + 20, sliderWidth, sliderHeight);
    }
    return;
  }

  // --- Default Layout ---
  int y = 30;

  for (int i = 0; i < comboBoxes.size(); ++i) {
    comboLabels[i]->setBounds(10, y, getWidth() - 20, 20);
    y += 20;
    comboBoxes[i]->setBounds(10, y, getWidth() - 20, 24);
    y += 30;
  }

  for (int i = 0; i < toggles.size(); ++i) {
    toggles[i]->setBounds(10, y, getWidth() - 20, 24);
    y += 30;
  }

  int sliderWidth = (getWidth() - 20) / 2;
  int sliderHeight = 60;

  for (int i = 0; i < sliders.size(); ++i) {
    int row = i / 2;
    int col = i % 2;

    int x = 10 + col * sliderWidth;
    int localY = y + row * (sliderHeight + 20);

    sliderLabels[i]->setBounds(x, localY, sliderWidth, 20);
    sliders[i]->setBounds(x, localY + 20, sliderWidth, sliderHeight);
  }
}

void ModuleComponent::mouseDown(const juce::MouseEvent &e) {
  dragger.startDraggingComponent(this, e);
}

void ModuleComponent::mouseDrag(const juce::MouseEvent &e) {
  dragger.dragComponent(this, e, nullptr);
  if (auto *p = getParentComponent())
    p->repaint();
}
