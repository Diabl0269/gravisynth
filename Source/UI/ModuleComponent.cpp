#include "ModuleComponent.h"
#include "../Modules/ADSRModule.h"
#include "../Modules/AttenuverterModule.h"
#include "../Modules/FilterModule.h"
#include "../Modules/LFOModule.h"
#include "../Modules/MidiKeyboardModule.h"
#include "../Modules/OscillatorModule.h"
#include "../Modules/SequencerModule.h"
#include "../Modules/VCAModule.h"
#include "GraphEditor.h"

ModuleComponent::ModuleComponent(juce::AudioProcessor* m, juce::AudioProcessorGraph::NodeID nodeId, GraphEditor& owner)
    : module(m)
    , nodeId(nodeId)
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
    if (module->getName().startsWith("Sequencer")) {
        setSize(510, 380); // 8 cols * 60 + margins, 3 rows
        return;
    }

    updateLayout();
}

void ModuleComponent::updateLayout() {
    if (module->getName().startsWith("Mod Slot") || module->getName().startsWith("Attenuverter")) {
        setSize(40, 40);
        if (sliders.size() > 0) {
            sliders[0]->setBounds(0, 0, 40, 40);
            sliders[0]->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            sliders[0]->setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::yellow);
            if (sliderLabels.size() > 0)
                sliderLabels[0]->setVisible(false);
        }
        return;
    }

    if (module->getName().startsWith("Sequencer")) {
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
    if (module->getName().startsWith("Mod Slot") || module->getName().startsWith("Attenuverter")) {
        return; // Transparent background, no ports, no header
    }

    g.fillAll(juce::Colours::lightgrey);

    // Highlight Active Step (Sequencer only)
    if (auto* seq = dynamic_cast<SequencerModule*>(module)) {
        int activeStep = seq->currentActiveStep.load();
        // Coordinates match resized()
        int startX = 10;
        int stepWidth = 60;
        int x = startX + activeStep * stepWidth;

        g.setColour(juce::Colours::yellow.withAlpha(0.3f));
        g.fillRect(x, 110, stepWidth - 5, 220); // Cover Gate+Pitch+F.Env area
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
    bool midiOutDrawn = false; // Reintroduced
                               // MIDI Output (Top Right if produces midi)
    if (module->producesMidi()) {
        g.setColour(juce::Colours::white);
        auto p = juce::Point<int>(getWidth() - 10, 30); // Top right, below header
        g.fillEllipse(p.x - 5, p.y - 5, 10, 10);
        g.drawText("Midi Out", p.x - 65, p.y - 5, 60, 10, juce::Justification::right, false);
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

        // 1. Check for dynamic CV labels from ModulationTargets
        bool foundTarget = false;
        if (auto* modBase = dynamic_cast<ModuleBase*>(module)) {
            auto targets = modBase->getModulationTargets();
            for (const auto& target : targets) {
                if (target.channelIndex == i) {
                    label = target.name + " CV";
                    foundTarget = true;
                    break;
                }
            }
        }

        // 2. Fallback to hardcoded Audio labels for non-CV ports
        if (!foundTarget) {
            if (auto* filter = dynamic_cast<FilterModule*>(module)) {
                if (i == 0)
                    label = "Audio";
            } else if (auto* vca = dynamic_cast<VCAModule*>(module)) {
                if (i == 0)
                    label = "Audio";
            } else if (module->getName().contains("Delay") || module->getName().contains("Distortion") ||
                       module->getName().contains("Reverb")) {
                if (i == 0)
                    label = "Audio L";
                else if (i == 1)
                    label = "Audio R";
            }
        }

        g.drawText(label, p.x + 10, p.y - 10, 60, 20, juce::Justification::left, false);
    }

    // Outputs
    int audioOutStartIndex = midiOutDrawn ? 1 : 0;
    for (int i = 0; i < numOuts; ++i) {
        auto p = getPortCenter(i, false);
        g.fillEllipse(p.x - 5, p.y - 5, 10, 10);

        juce::String label = "Out " + juce::String(i);
        if (auto* lfo = dynamic_cast<LFOModule*>(module)) {
            label = "CV Out " + juce::String(i + 1);
        }
        g.drawText(label, p.x - 70, p.y - 10, 60, 20, juce::Justification::right, false);
    }

    // 4. Output Connection Badge (Serum-style)
    auto routings = owner.getAudioEngine().getActiveModRoutings();
    int connectionCount = 0;
    for (const auto& r : routings) {
        // ONLY count if destNodeID is valid (meaning it's fully connected)
        if (r.sourceNodeID == nodeId && r.destNodeID != juce::AudioProcessorGraph::NodeID()) {
            connectionCount++;
        }
    }

    if (connectionCount > 0) {
        auto portPos = getPortCenter(0, false); // Output port
        float badgeSize = 22.0f;
        // Position badge ABOVE the output port circle specifically to avoid horizontal text overlap
        juce::Rectangle<float> badgeRect(portPos.x - badgeSize / 2.0f, portPos.y - 30.0f, badgeSize, badgeSize);

        g.setColour(juce::Colours::orange);
        g.fillEllipse(badgeRect);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(juce::String(connectionCount), badgeRect.toNearestInt(), juce::Justification::centred);
    }
}

juce::Point<int> ModuleComponent::getPortCenter(int index, bool isInput) {
    if (module->getName().startsWith("Mod Slot") || module->getName().startsWith("Attenuverter")) {
        return {getWidth() / 2, getHeight() / 2};
    }

    int yStep = 20;
    int headerHeight = 30;

    int portOffset = 0;
    if (module->producesMidi()) {
        portOffset = 20; // Additional offset for all ports if MIDI out is present, to avoid collision with MIDI Out at
                         // (getWidth() - 10, 30)
    }

    if (isInput) {
        return {10, headerHeight + portOffset + index * yStep + 20}; // Left side, apply offset
    } else {
        // No additional midiOffset for outputs here, as MIDI out is now fixed.
        return {getWidth() - 10, headerHeight + portOffset + index * yStep + 20}; // Right side, apply offset
    }
}

std::optional<ModuleComponent::Port> ModuleComponent::getPortForPoint(juce::Point<int> localPoint) {
    if (module->getName().startsWith("Mod Slot") || module->getName().startsWith("Attenuverter")) {
        return std::nullopt; // Users cannot manually drag connections from the smart wire knob
    }

    int numIns = module->getTotalNumInputChannels();
    int numOuts = module->getTotalNumOutputChannels();

    // Check for MIDI Output at fixed top-right position
    if (module->producesMidi()) {
        auto p = juce::Point<int>(getWidth() - 10, 30); // Matches paint()
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10},
                        juce::AudioProcessorGraph::midiChannelIndex,
                        false,
                        true}; // Index 0, Output, IsMidi
        }
    }

    // MIDI Input detection (Top Left)
    if (module->acceptsMidi()) {
        auto p = juce::Point<int>(10, 30); // Top left near header
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{
                {p.x - 5, p.y - 5, 10, 10}, juce::AudioProcessorGraph::midiChannelIndex, true, true}; // MIDI Input
        }
    }

    // Inputs
    for (int i = 0; i < numIns; ++i) {
        auto p = getPortCenter(i, true);
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10}, i, true, false};
        }
    }

    // Outputs (audio outputs)
    for (int i = 0; i < numOuts; ++i) {
        auto p = getPortCenter(i, false);
        if (localPoint.getDistanceFrom(p) < 10) {
            return Port{{p.x - 5, p.y - 5, 10, 10}, i, false, false};
        }
    }

    return std::nullopt;
}

void ModuleComponent::resized() {
    if (module->getName().startsWith("Sequencer")) {
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
    if (module->getName().contains("MIDI Keyboard") || module->getName().contains("MidiKeyboard")) {
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

    // Helper lambda to build the unified modulation menu
    auto addModulationSubMenu = [this](juce::PopupMenu& m, const std::vector<AudioEngine::ModRoutingInfo>& routings) {
        int bypassedCount = 0;
        int activeCount = 0;
        std::vector<AudioEngine::ModRoutingInfo> myRoutings;

        for (const auto& r : routings) {
            if (r.sourceNodeID == nodeId && r.destNodeID != juce::AudioProcessorGraph::NodeID()) {
                myRoutings.push_back(r);
                if (r.isBypassed)
                    bypassedCount++;
                else
                    activeCount++;
            }
        }

        if (!myRoutings.empty()) {
            m.addSeparator();
            m.addSectionHeader("Modulation Destinations");

            for (const auto& r : myRoutings) {
                auto* dstNode = owner.getGraph().getNodeForId(r.destNodeID);
                if (!dstNode)
                    continue;

                juce::String dstName = dstNode->getProcessor()->getName();
                if (auto* modBase = dynamic_cast<ModuleBase*>(dstNode->getProcessor())) {
                    auto targets = modBase->getModulationTargets();
                    if (r.destChannelIndex < (int)targets.size()) {
                        dstName += " -> " + targets[r.destChannelIndex].name;
                    }
                }

                // Tick shows active status
                m.addItem(juce::PopupMenu::Item(dstName).setTicked(!r.isBypassed).setAction([this, r] {
                    owner.getAudioEngine().toggleModBypass(r.attenuverterNodeID);
                    repaint();
                }));
            }

            m.addSeparator();
            if (bypassedCount > 0) {
                m.addItem("Activate All Destinations", [this, myRoutings] {
                    for (const auto& r : myRoutings) {
                        if (r.isBypassed)
                            owner.getAudioEngine().toggleModBypass(r.attenuverterNodeID);
                    }
                    repaint();
                });
            }
            if (activeCount > 0) {
                m.addItem("Bypass All Destinations", [this, myRoutings] {
                    for (const auto& r : myRoutings) {
                        if (!r.isBypassed)
                            owner.getAudioEngine().toggleModBypass(r.attenuverterNodeID);
                    }
                    repaint();
                });
            }
            m.addItem("Remove All Destinations", [this, myRoutings] {
                for (const auto& r : myRoutings) {
                    owner.getAudioEngine().removeModRouting(r.attenuverterNodeID);
                }
                repaint();
            });
        }
    };

    if (port) {
        if (e.mods.isPopupMenu()) {
            juce::PopupMenu m;
            m.addItem("Disconnect",
                      [this, port] { owner.disconnectPort(this, port->index, port->isInput, port->isMidi); });

            // If it's an output port, also show destination management
            if (!port->isInput && !port->isMidi) {
                addModulationSubMenu(m, owner.getAudioEngine().getActiveModRoutings());
            }

            m.showMenuAsync(juce::PopupMenu::Options());
        } else {
            owner.beginConnectionDrag(this, port->index, port->isInput, port->isMidi, e.getScreenPosition());
        }
        return;
    } else {
        // Click on Body
        if (module->getName().startsWith("Mod Slot") || module->getName().startsWith("Attenuverter")) {
            return;
        }

        if (e.mods.isRightButtonDown()) {
            juce::PopupMenu menu;

            // Shared modulation logic
            addModulationSubMenu(menu, owner.getAudioEngine().getActiveModRoutings());

            menu.addSeparator();
            menu.addItem("Delete Module", [this] { owner.deleteModule(this); });

            menu.showMenuAsync(juce::PopupMenu::Options());
            return;
        }

        dragger.startDraggingComponent(this, e);
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
