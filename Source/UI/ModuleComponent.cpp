#include "ModuleComponent.h"
#include "../Modules/SequencerModule.h"
#include "GraphEditor.h"

ModuleComponent::ModuleComponent(juce::AudioProcessor* m, GraphEditor& owner)
    : module(m)
    , owner(owner) {

    if (auto* modBase = dynamic_cast<ModuleBase*>(module)) {
        if (auto* vb = modBase->getVisualBuffer()) {
            scopeComponent = std::make_unique<ScopeComponent>(*vb);
            addAndMakeVisible(scopeComponent.get());

            scopeToggle = std::make_unique<juce::ToggleButton>("Show Scope");
            scopeToggle->setToggleState(true, juce::dontSendNotification);
            scopeToggle->onClick = [this] {
                scopeComponent->setVisible(scopeToggle->getToggleState());
                updateLayout();
            };
            addAndMakeVisible(scopeToggle.get());
        }
    }

    createControls();
    startTimerHz(30); // 30 FPS for step visualization
}

ModuleComponent::~ModuleComponent() { stopTimer(); }
void ModuleComponent::timerCallback() { repaint(); }

void ModuleComponent::createControls() {
    // Auto-UI
    if (auto* midiKeyboard = dynamic_cast<MidiKeyboardModule*>(module)) {
        keyboardComponent = std::make_unique<juce::MidiKeyboardComponent>(
            midiKeyboard->getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard);
        addAndMakeVisible(keyboardComponent.get());
    } else {
        const auto& params = module->getParameters();

        for (auto* param : params) {
            if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)) {
                auto* combo = comboBoxes.add(new juce::ComboBox());
                combo->addItemList(choiceParam->choices, 1);
                addAndMakeVisible(combo);

                auto* attach = comboAttachments.add(new juce::ComboBoxParameterAttachment(*choiceParam, *combo));

                auto* label = comboLabels.add(new juce::Label(param->getName(100), param->getName(100)));
                addAndMakeVisible(label);
            } else if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
                auto* slider = sliders.add(new juce::Slider());
                slider->setComponentID(param->getName(100)); // ID for lookup
                if (module->getName().contains("ADSR") || module->getName().contains("Env")) {
                    slider->setSliderStyle(juce::Slider::LinearVertical);
                    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
                } else {
                    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
                }
                addAndMakeVisible(slider);

                auto* attach = sliderAttachments.add(new juce::SliderParameterAttachment(*floatParam, *slider));

                auto* label = sliderLabels.add(new juce::Label(param->getName(100), param->getName(100)));
                label->setJustificationType(juce::Justification::centred);
                addAndMakeVisible(label);
            } else if (auto* intParam = dynamic_cast<juce::AudioParameterInt*>(param)) {
                auto* slider = sliders.add(new juce::Slider());
                slider->setComponentID(param->getName(100)); // ID for lookup
                slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
                // slider->setRange(intParam->getRange().start,
                // intParam->getRange().end, 1.0); // Attachment handles range
                addAndMakeVisible(slider);

                auto* attach = sliderAttachments.add(new juce::SliderParameterAttachment(*intParam, *slider));

                auto* label = sliderLabels.add(new juce::Label(param->getName(100), param->getName(100)));
                label->setJustificationType(juce::Justification::centred);
                addAndMakeVisible(label);
            } else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param)) {
                auto* toggle = toggles.add(new juce::ToggleButton(boolParam->getName(100)));
                toggle->setComponentID(boolParam->getName(100)); // ID for Lookup
                addAndMakeVisible(toggle);

                auto* attach = buttonAttachments.add(new juce::ButtonParameterAttachment(*boolParam, *toggle));
            }
        }
    }

    // Auto-resize
    if (module->getName() == "Sequencer") {
        setSize(510, 380); // 8 cols * 60 + margins, 3 rows
        return;
    }

    updateLayout();
}

void ModuleComponent::updateLayout() {
    if (module->getName() == "Sequencer") {
        setSize(510, 380);
        return;
    }

    if (module->getName().contains("ADSR") || module->getName().contains("Env")) {
        setSize(220 + 60, 180); // 180 is height from ADSR Layout
        return;
    }

    int contentHeight = 40; // Header
    contentHeight += comboBoxes.size() * 50;
    contentHeight += toggles.size() * 30;

    if (scopeToggle)
        contentHeight += 30;

    int numSliders = sliders.size();
    int rows = (numSliders + 1) / 2;
    contentHeight += rows * 80;

    if (scopeComponent && scopeComponent->isVisible())
        contentHeight += 110;

    setSize(280, std::max(100, contentHeight + 20));
    resized();
}

void ModuleComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::lightgrey);

    // Highlight Active Step (Sequencer only)
    if (module->getName() == "Sequencer") {
        if (auto* seq = dynamic_cast<SequencerModule*>(module)) {
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
    g.drawText(module->getName(), 0, 0, getWidth(), 24, juce::Justification::centred, true);

    // --- PORTS ---
    int numIns = module->getTotalNumInputChannels();
    int numOuts = module->getTotalNumOutputChannels();

    // Flag to track if MIDI out has been drawn to adjust y position for audio ports
    bool midiOutDrawn = false;

    if (module->producesMidi()) { // General check for MIDI producing modules
        g.setColour(juce::Colours::white);
        auto p = getPortCenter(0, false); // Assuming MIDI Out is always port 0
        g.fillEllipse(p.x - 5, p.y - 5, 10, 10);
        g.drawText("Midi Out", p.x - 65, p.y - 5, 60, 10, juce::Justification::right, false);
        midiOutDrawn = true;
    }

    // MIDI Input (Top Left if accepts midi components)
    if (module->acceptsMidi()) {
        g.setColour(juce::Colours::white);
        auto p = juce::Point<int>(10, 30); // Top left near header
        g.fillEllipse(p.x - 5, p.y - 5, 10, 10);
        g.drawText("Midi In", p.x + 10, p.y - 5, 60, 10, juce::Justification::left, false);
    }

    // Inputs
    g.setColour(juce::Colours::yellow);
    for (int i = 0; i < numIns; ++i) {
        auto p = getPortCenter(i, true);
        g.fillEllipse(p.x - 5, p.y - 5, 10, 10);

        juce::String label = "In " + juce::String(i);
        // Custom labels for Filter
        if (module->getName() == "Filter") {
            if (i == 0)
                label = "Audio";
            if (i == 1)
                label = "Freq CV";
        }
        if (module->getName() == "VCA") {
            if (i == 0)
                label = "Audio";
            if (i == 1)
                label = "CV";
        }

        g.drawText(label, p.x + 10, p.y - 10, 60, 20, juce::Justification::left, false);
    }

    // Outputs
    // Only draw audio outputs if MIDI out hasn't been drawn in the same general area (to prevent overlap)
    // For now, we assume MIDI out takes the "first" audio output slot.
    // A more robust solution would involve explicit port mapping.
    int audioOutStartIndex = midiOutDrawn ? 1 : 0;
    for (int i = audioOutStartIndex; i < numOuts + audioOutStartIndex;
         ++i) { // Adjust index for display if midi out is present
        auto p = getPortCenter(i, false);
        g.fillEllipse(p.x - 5, p.y - 5, 10, 10);

        juce::String label = "Out " + juce::String(i);
        g.drawText(label, p.x - 70, p.y - 10, 60, 20, juce::Justification::right, false);
    }
}

juce::Point<int> ModuleComponent::getPortCenter(int index, bool isInput) {
    int yStep = 20;
    int headerHeight = 30;

    int midiOffset = 0;
    if (!isInput && module->producesMidi()) {
        midiOffset = 1; // Shift audio outputs down by one slot if MIDI out is present
    }

    if (isInput) {
        return {10, headerHeight + index * yStep + 20}; // Left side
    } else {
        return {getWidth() - 10, headerHeight + (index + midiOffset) * yStep + 20}; // Right side, apply offset
    }
}

std::optional<ModuleComponent::Port> ModuleComponent::getPortForPoint(juce::Point<int> localPoint) {
    int numIns = module->getTotalNumInputChannels();
    int numOuts = module->getTotalNumOutputChannels();

    // Check for MIDI Output
    if (module->producesMidi()) {
        auto p = getPortCenter(0, false); // MIDI Out is always index 0, on the right
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10},
                        juce::AudioProcessorGraph::midiChannelIndex,
                        false,
                        true}; // Index 0, Output, IsMidi
        }
    }

    // MIDI Input detection
    if (module->acceptsMidi()) {
        auto p = juce::Point<int>(10, 30); // Top left near header
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10},
                        juce::AudioProcessorGraph::midiChannelIndex,
                        true,
                        true}; // MIDI Input, index -1 (or special index)
        }
    }

    // Inputs
    for (int i = 0; i < numIns; ++i) {
        auto p = getPortCenter(i, true);
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10}, i, true, false};
        }
    }

    // Outputs (audio outputs, shifted if MIDI out is present)
    int audioOutOffset = module->producesMidi() ? 1 : 0;
    for (int i = 0; i < numOuts; ++i) {
        auto p = getPortCenter(i, false); // getPortCenter already accounts for midiOffset
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10}, i, false, false};
        }
    }

    return std::nullopt;
}

void ModuleComponent::resized() {
    if (module->getName() == "Sequencer") {
        // --- Sequencer Specific Layout ---
        int x = 10;
        int y = 30;

        // Top Row: Run and BPM
        for (auto* toggle : toggles) {
            if (toggle->getComponentID().equalsIgnoreCase("run")) {
                toggle->setBounds(x + 30, y, 60, 24); // Add margin
                x += 70;
            }
        }

        for (int i = 0; i < sliders.size(); ++i) {
            if (sliders[i]->getComponentID().equalsIgnoreCase("bpm")) {
                sliderLabels[i]->setBounds(x + 20, y, 60, 20); // Add margin
                sliders[i]->setBounds(x + 20, y + 20, 60, 50);
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
        int margin = 30;                // Side margins for ports
        setSize(220 + margin * 2, 180); // Increase width
        int y = 30;
        int contentWidth = getWidth() - margin * 2;
        int sliderWidth = 50;
        int sliderHeight = 120;

        // We expect 4 sliders: A, D, S, R
        for (int i = 0; i < sliders.size(); ++i) {
            int x = margin + 10 + i * sliderWidth;
            sliderLabels[i]->setBounds(x, y, sliderWidth, 20);
            sliders[i]->setBounds(x, y + 20, sliderWidth, sliderHeight);
        }
        return;
    }

    // --- MIDI Keyboard Layout ---
    if (module->getName() == "MIDI Keyboard") {
        setSize(500, 150); // Appropriate size for a keyboard
        if (keyboardComponent) {
            keyboardComponent->setBounds(10, 50, getWidth() - 20, getHeight() - 60);
        }
        return;
    }

    // --- Default Layout ---
    int y = 30;
    // Increase top y if MIDI IN is present to avoid overlap
    if (module->acceptsMidi())
        y += 30;

    int margin = 70; // Wider margin for labels
    int contentWidth = getWidth() - (margin * 2);

    for (int i = 0; i < comboBoxes.size(); ++i) {
        comboLabels[i]->setBounds(margin, y, contentWidth, 20);
        y += 20;
        comboBoxes[i]->setBounds(margin, y, contentWidth, 24);
        y += 30;
    }

    for (int i = 0; i < toggles.size(); ++i) {
        toggles[i]->setBounds(margin, y, contentWidth, 24);
        y += 30;
    }

    int sliderWidth = contentWidth / 2;
    int sliderHeight = 60;

    for (int i = 0; i < sliders.size(); ++i) {
        int row = i / 2;
        int col = i % 2;

        int x = margin + col * sliderWidth;
        int localY = y + row * (sliderHeight + 20);

        sliderLabels[i]->setBounds(x, localY, sliderWidth, 20);
        sliders[i]->setBounds(x, localY + 20, sliderWidth, sliderHeight);
    }

    // Update y to the end of sliders for scope toggle/scope
    int finalSlidersRow = (sliders.size() + 1) / 2;
    y += finalSlidersRow * (sliderHeight + 20);

    if (scopeToggle) {
        scopeToggle->setBounds(margin, y, contentWidth, 24);
        y += 30;
    }

    if (scopeComponent && scopeComponent->isVisible()) {
        scopeComponent->setBounds(10, y, getWidth() - 20, 100);
    }
}

void ModuleComponent::mouseDown(const juce::MouseEvent& e) {
    auto port = getPortForPoint(e.getPosition());
    if (port) {
        if (e.mods.isPopupMenu()) {
            // Right click -> Disconnect
            // Show menu? Or just disconnect?
            // User asked for "way to disconnect". Instant disconnect is fast.
            // Or a menu "Disconnect".
            juce::PopupMenu m;
            m.addItem("Disconnect",
                      [this, port] { owner.disconnectPort(this, port->index, port->isInput, port->isMidi); });

            m.showMenuAsync(juce::PopupMenu::Options());
        } else {
            // Start Connection Drag
            owner.beginConnectionDrag(this, port->index, port->isInput, port->isMidi, e.getScreenPosition());
        }
    } else {
        // Click on Body
        if (e.mods.isPopupMenu()) {
            juce::PopupMenu m;
            m.addItem("Delete Module", [this] { owner.deleteModule(this); });
            m.showMenuAsync(juce::PopupMenu::Options());
        } else {
            dragger.startDraggingComponent(this, e);
        }
    }
}

void ModuleComponent::moved() { owner.updateModulePosition(this); }

void ModuleComponent::mouseDrag(const juce::MouseEvent& e) {
    if (getPortForPoint(e.getMouseDownPosition())) {
        owner.dragConnection(e.getScreenPosition());
    } else {
        dragger.dragComponent(this, e, nullptr);
        if (auto* p = getParentComponent())
            p->repaint();
    }
}

void ModuleComponent::mouseUp(const juce::MouseEvent& e) {
    if (getPortForPoint(e.getMouseDownPosition())) {
        owner.endConnectionDrag(e.getScreenPosition());
    }
}
