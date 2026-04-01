#include "ModMatrixComponent.h"
#include "../Modules/AttenuverterModule.h"

ModMatrixComponent::ModMatrixComponent(AudioEngine& engine)
    : audioEngine(engine) {
    startTimerHz(5); // Refresh routing array 5 times a second
}

ModMatrixComponent::~ModMatrixComponent() { stopTimer(); }

void ModMatrixComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey.darker());
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);

    auto titleArea = getLocalBounds().removeFromTop(30);
    g.fillRect(titleArea);
    g.setColour(juce::Colours::black);
    g.drawText("MATRIX", titleArea, juce::Justification::centred, true);

    if (rows.empty()) {
        g.setColour(juce::Colours::grey);
        g.drawText("No routings active.", getLocalBounds().withTrimmedTop(30), juce::Justification::centred, true);
    } else {
        g.setColour(juce::Colours::white);
        int y = 40;
        for (auto& row : rows) {
            juce::String text = row->sourceName + " -> " + row->destName;
            g.drawText(text, 10, y, getWidth() - 20, 20, juce::Justification::left, true);
            y += 50;
        }
    }
}

void ModMatrixComponent::resized() {
    int y = 60; // 40 for text + 20
    for (auto& row : rows) {
        if (row->amountSlider) {
            row->amountSlider->setBounds(10, y, getWidth() - 20, 20);
        }
        y += 50;
    }
}

void ModMatrixComponent::timerCallback() {
    auto& graph = audioEngine.getGraph();

    // Check if the number of Attenuverters has changed or connections changed.
    std::vector<juce::AudioProcessorGraph::NodeID> currentAttenIds;

    for (auto* n : graph.getNodes()) {
        if (n->getProcessor()->getName() == "Attenuverter") {
            currentAttenIds.push_back(n->nodeID);
        }
    }

    bool needsUpdate = false;
    if (currentAttenIds.size() != rows.size()) {
        needsUpdate = true;
    } else {
        for (size_t i = 0; i < currentAttenIds.size(); ++i) {
            if (currentAttenIds[i] != rows[i]->nodeId) {
                needsUpdate = true;
                break;
            }
        }
    }

    if (needsUpdate) {
        rows.clear();
        for (auto id : currentAttenIds) {
            auto* node = graph.getNodeForId(id);
            if (!node)
                continue;

            juce::String srcName = "?";
            juce::String dstName = "?";

            // Find connections
            for (auto& c : graph.getConnections()) {
                if (c.destination.nodeID == id) {
                    if (auto* srcNode = graph.getNodeForId(c.source.nodeID)) {
                        srcName = srcNode->getProcessor()->getName();
                    }
                }
                if (c.source.nodeID == id) {
                    if (auto* dstNode = graph.getNodeForId(c.destination.nodeID)) {
                        dstName = dstNode->getProcessor()->getName();
                        if (dstName == "Filter") {
                            if (c.destination.channelIndex == 1)
                                dstName += " Cutoff";
                            else if (c.destination.channelIndex == 2)
                                dstName += " Res";
                            else if (c.destination.channelIndex == 3)
                                dstName += " Drive";
                        } else if (dstName == "VCA") {
                            if (c.destination.channelIndex == 1)
                                dstName += " CV";
                        }
                    }
                }
            }

            auto row = std::make_unique<ModRow>();
            row->nodeId = id;
            row->sourceName = srcName;
            row->destName = dstName;

            row->amountSlider = std::make_unique<juce::Slider>();
            row->amountSlider->setSliderStyle(juce::Slider::LinearHorizontal);
            row->amountSlider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            row->amountSlider->setPopupDisplayEnabled(true, false, nullptr);

            // Attach slider
            auto& params = node->getProcessor()->getParameters();
            if (!params.isEmpty()) {
                if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(params[0])) {
                    row->attachment =
                        std::make_unique<juce::SliderParameterAttachment>(*floatParam, *row->amountSlider);
                }
            }

            addAndMakeVisible(row->amountSlider.get());
            rows.push_back(std::move(row));
        }
        resized();
        repaint();
    }
}
