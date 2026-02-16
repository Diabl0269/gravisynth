#include "GraphEditor.h"
#include "../Modules/ADSRModule.h"
#include "../Modules/FX/DelayModule.h"
#include "../Modules/FX/DistortionModule.h"
#include "../Modules/FX/ReverbModule.h"
#include "../Modules/FilterModule.h"
#include "../Modules/LFOModule.h"
#include "../Modules/MidiKeyboardModule.h"
#include "../Modules/OscillatorModule.h"
#include "../Modules/SequencerModule.h"
#include "../Modules/VCAModule.h"
#include "ModuleComponent.h"

GraphEditor::GraphEditor(AudioEngine& engine)
    : audioEngine(engine)
    , content(*this) {
    addAndMakeVisible(content);
    content.setInterceptsMouseClicks(false, true); // Fallback clicks to parent
    startTimerHz(30);
}

GraphEditor::~GraphEditor() {}

GraphEditor::GraphContentComponent::GraphContentComponent(GraphEditor& ed)
    : editor(ed) {}

void GraphEditor::GraphContentComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey);

    // Draw connections
    auto& graph = editor.audioEngine.getGraph();
    g.setColour(juce::Colours::yellow);

    for (auto& connection : graph.getConnections()) {
        juce::Point<int> p1, p2;
        bool found1 = false;
        bool found2 = false;

        for (auto* comp : moduleComponents) {
            if (auto* module = comp->getModule()) {
                auto* node1 = graph.getNodeForId(connection.source.nodeID);
                auto* node2 = graph.getNodeForId(connection.destination.nodeID);

                if (node1 && node1->getProcessor() == module) {
                    juce::Point<int> localP;
                    if (connection.source.channelIndex == juce::AudioProcessorGraph::midiChannelIndex) {
                        localP = comp->getPortCenter(0, false);
                    } else {
                        localP = comp->getPortCenter(connection.source.channelIndex, false);
                    }
                    p1 = comp->getBounds().getPosition() + localP;
                    found1 = true;
                }
                if (node2 && node2->getProcessor() == module) {
                    juce::Point<int> localP;
                    if (connection.destination.channelIndex == juce::AudioProcessorGraph::midiChannelIndex) {
                        localP = juce::Point<int>(10, 30);
                    } else {
                        localP = comp->getPortCenter(connection.destination.channelIndex, true);
                    }
                    p2 = comp->getBounds().getPosition() + localP;
                    found2 = true;
                }
            }
        }

        if (found1 && found2)
            g.drawLine(p1.toFloat().x, p1.toFloat().y, p2.toFloat().x, p2.toFloat().y, 2.0f);
    }

    // Draw Line being dragged
    if (editor.isDraggingConnection) {
        g.setColour(juce::Colours::white);
        if (editor.dragSourceModule) {
            juce::Point<int> p;
            if (editor.dragSourceIsMidi) {
                if (editor.dragSourceIsInput)
                    p = juce::Point<int>(10, 30);
                else
                    p = editor.dragSourceModule->getPortCenter(0, false);
            } else {
                p = editor.dragSourceModule->getPortCenter(editor.dragSourceChannel, editor.dragSourceIsInput);
            }

            auto posInContent = editor.dragSourceModule->getBounds().getPosition() + p;
            auto mouseInContent = getLocalPoint(&(editor), editor.dragCurrentPos);

            // Since 'content' is transformed, we need to handle coordinates
            // carefully. But if this 'paint' is called on content, and we use
            // getLocalPoint(editor, ...) it should be transformed back. Actually,
            // easier: editor.dragCurrentPos is screen pos.
            auto mouseLocal = getLocalPoint(nullptr, editor.dragCurrentPos);

            g.drawLine(posInContent.toFloat().x, posInContent.toFloat().y, mouseLocal.toFloat().x,
                       mouseLocal.toFloat().y, 3.0f);
        }
    }

    g.setColour(juce::Colours::white);
    g.drawText("Gravisynth (Double click to refresh)", getLocalBounds().removeFromTop(20), juce::Justification::centred,
               true);
}

void GraphEditor::GraphContentComponent::resized() {}

void GraphEditor::beginConnectionDrag(ModuleComponent* sourceModule, int channelIndex, bool isInput, bool isMidi,
                                      juce::Point<int> screenPos) {
    isDraggingConnection = true;
    dragSourceModule = sourceModule;
    dragSourceChannel = channelIndex;
    dragSourceIsInput = isInput;
    dragSourceIsMidi = isMidi;
    dragCurrentPos = screenPos;
    content.repaint();
}

void GraphEditor::dragConnection(juce::Point<int> screenPos) {
    if (!isDraggingConnection)
        return;
    dragCurrentPos = screenPos;
    content.repaint();
}

void GraphEditor::endConnectionDrag(juce::Point<int> screenPos) {
    if (!isDraggingConnection)
        return;

    // Hit test for target in content space
    auto contentPos = content.getLocalPoint(nullptr, screenPos);

    for (auto* comp : content.getModules()) {
        auto localPos = comp->getLocalPoint(nullptr, screenPos);
        auto port = comp->getPortForPoint(localPos);

        if (port) {
            if (comp == dragSourceModule)
                continue;
            if (port->isInput == dragSourceIsInput)
                continue;
            if (port->isMidi != dragSourceIsMidi)
                continue;

            auto& graph = audioEngine.getGraph();
            juce::AudioProcessorGraph::Node* srcNode = nullptr;
            juce::AudioProcessorGraph::Node* dstNode = nullptr;

            for (auto* n : graph.getNodes()) {
                if (n->getProcessor() == dragSourceModule->getModule())
                    srcNode = n;
                if (n->getProcessor() == comp->getModule())
                    dstNode = n;
            }

            if (srcNode && dstNode) {
                if (dragSourceIsMidi) {
                    graph.addConnection({{srcNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                                         {dstNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
                } else {
                    if (dragSourceIsInput) {
                        graph.addConnection({{dstNode->nodeID, port->index}, {srcNode->nodeID, dragSourceChannel}});
                    } else {
                        graph.addConnection({{srcNode->nodeID, dragSourceChannel}, {dstNode->nodeID, port->index}});
                    }
                }
            }
        }
    }

    isDraggingConnection = false;
    dragSourceModule = nullptr;
    content.repaint();
}

void GraphEditor::updateComponents() {
    content.getModules().clear();
    content.removeAllChildren();

    auto& graph = audioEngine.getGraph();

    for (auto* node : graph.getNodes()) {
        auto* processor = node->getProcessor();
        if (processor) {
            auto* comp = content.getModules().add(new ModuleComponent(processor, *this));
            content.addAndMakeVisible(comp);

            auto x = node->properties.getWithDefault("x", -1);
            auto y = node->properties.getWithDefault("y", -1);

            if (static_cast<int>(x) != -1 && static_cast<int>(y) != -1) {
                comp->setTopLeftPosition(static_cast<int>(x), static_cast<int>(y));
            } else {
                // Robust fallback for missing positions
                juce::String name = processor->getName();
                if (name == "Sequencer")
                    comp->setTopLeftPosition(10, 80);
                else if (name == "Oscillator")
                    comp->setTopLeftPosition(540, 50);
                else if (name == "Amp Env")
                    comp->setTopLeftPosition(540, 450);
                else if (name == "Filter")
                    comp->setTopLeftPosition(830, 50);
                else if (name == "Filter Env")
                    comp->setTopLeftPosition(830, 450);
                else if (name == "LFO")
                    comp->setTopLeftPosition(10, 500);
                else if (name == "VCA")
                    comp->setTopLeftPosition(1120, 50);
                else if (name.containsIgnoreCase("Output"))
                    comp->setTopLeftPosition(2250, 300);
                else if (name.containsIgnoreCase("Input"))
                    comp->setTopLeftPosition(10, 10);
                else
                    comp->setTopLeftPosition(100 + (content.getModules().size() * 30), 400);
            }
        }
    }
    repaint();
}

void GraphEditor::paint(juce::Graphics& g) {
    // GraphEditor itself can draw a background or overlay if needed
    // But content handles it now.
}

void GraphEditor::resized() { updateTransform(); }

void GraphEditor::updateTransform() {
    juce::AffineTransform t;
    t = t.translated(panOffset);
    t = t.scaled(zoomLevel, zoomLevel, getWidth() / 2.0f, getHeight() / 2.0f);

    content.setBounds(0, 0, 10000, 10000);
    content.setTransform(t);
    repaint();
}

void GraphEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {
    float oldZoom = zoomLevel;
    zoomLevel += wheel.deltaY * 0.1f * zoomLevel;
    zoomLevel = juce::jlimit(0.1f, 2.0f, zoomLevel);

    // Adjust pan to zoom around mouse position?
    // For simplicity just zoom around center for now.
    updateTransform();
}

void GraphEditor::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown()) {
        lastMousePos = e.getPosition();
    }
}

void GraphEditor::mouseDrag(const juce::MouseEvent& e) {
    if (e.mods.isLeftButtonDown() && !isDraggingConnection) {
        auto delta = e.getPosition() - lastMousePos;
        panOffset += delta.toFloat();
        lastMousePos = e.getPosition();
        updateTransform();
    }
}

void GraphEditor::updateModulePosition(ModuleComponent* module) {
    if (!module)
        return;
    for (auto* n : audioEngine.getGraph().getNodes()) {
        if (n->getProcessor() == module->getModule()) {
            n->properties.set("x", module->getX());
            n->properties.set("y", module->getY());
            break;
        }
    }
}

void GraphEditor::deleteModule(ModuleComponent* module) {
    auto& graph = audioEngine.getGraph();
    juce::AudioProcessorGraph::NodeID nodeId;

    // Find NodeID
    for (auto* n : graph.getNodes()) {
        if (n->getProcessor() == module->getModule()) {
            nodeId = n->nodeID;
            break;
        }
    }

    if (nodeId.uid == 0)
        return; // Not found

    graph.removeNode(nodeId);
    updateComponents(); // Rebuild moduleComponents
    repaint();
}

void GraphEditor::disconnectPort(ModuleComponent* module, int portIndex, bool isInput, bool isMidi) {
    auto& graph = audioEngine.getGraph();
    juce::AudioProcessorGraph::NodeID nodeId;

    // Find NodeID
    for (auto* n : graph.getNodes()) {
        if (n->getProcessor() == module->getModule()) {
            nodeId = n->nodeID;
            break;
        }
    }

    if (nodeId.uid == 0)
        return; // Not found

    // Collect connections to remove (can't remove while iterating)
    std::vector<juce::AudioProcessorGraph::Connection> toRemove;

    int targetChannel = isMidi ? juce::AudioProcessorGraph::midiChannelIndex : portIndex;

    for (auto& c : graph.getConnections()) {
        if (isInput) {
            if (c.destination.nodeID == nodeId && c.destination.channelIndex == targetChannel) {
                toRemove.push_back(c);
            }
        } else {
            if (c.source.nodeID == nodeId && c.source.channelIndex == targetChannel) {
                toRemove.push_back(c);
            }
        }
    }

    for (auto& c : toRemove) {
        graph.removeConnection(c);
    }

    repaint();
}

void GraphEditor::timerCallback() {
    // empty for now, or use for other periodic updates
}

bool GraphEditor::isInterestedInDragSource(const SourceDetails& dragSourceDetails) { return true; }

void GraphEditor::itemDropped(const SourceDetails& dragSourceDetails) {
    juce::String name = dragSourceDetails.description.toString();
    std::unique_ptr<juce::AudioProcessor> newProcessor;

    if (name == "Oscillator")
        newProcessor = std::make_unique<OscillatorModule>();
    else if (name == "Filter")
        newProcessor = std::make_unique<FilterModule>();
    else if (name == "ADSR")
        newProcessor = std::make_unique<ADSRModule>();
    else if (name == "VCA")
        newProcessor = std::make_unique<VCAModule>();
    else if (name == "Sequencer")
        newProcessor = std::make_unique<SequencerModule>();
    else if (name == "LFO")
        newProcessor = std::make_unique<LFOModule>();
    else if (name == "Distortion")
        newProcessor = std::make_unique<DistortionModule>();
    else if (name == "Delay")
        newProcessor = std::make_unique<DelayModule>();
    else if (name == "Reverb")
        newProcessor = std::make_unique<ReverbModule>();
    else if (name == "MidiKeyboard")
        newProcessor = std::make_unique<MidiKeyboardModule>();

    if (newProcessor) {
        auto node = audioEngine.getGraph().addNode(std::move(newProcessor));
        if (node) {
            // Convert drop position to content space
            auto dropPos = content.getLocalPoint(this, dragSourceDetails.localPosition);
            node->properties.set("x", dropPos.x);
            node->properties.set("y", dropPos.y);

            updateComponents();
        }
    }
}

void GraphEditor::savePreset(juce::File file) {
    auto& graph = audioEngine.getGraph();
    juce::XmlElement xml("GRAVISYNTH_PATCH");

    // 1. Nodes
    auto* nodesElement = xml.createNewChildElement("NODES");
    for (auto node : graph.getNodes()) {
        auto* nodeElement = nodesElement->createNewChildElement("NODE");
        nodeElement->setAttribute("id", (int)node->nodeID.uid);
        nodeElement->setAttribute("type", node->getProcessor()->getName());
        nodeElement->setAttribute("x", node->properties["x"].toString());
        nodeElement->setAttribute("y", node->properties["y"].toString());

        juce::MemoryBlock stateData;
        node->getProcessor()->getStateInformation(stateData);
        nodeElement->setAttribute("state", stateData.toBase64Encoding());
    }

    // 2. Connections
    auto* connectionsElement = xml.createNewChildElement("CONNECTIONS");
    for (auto connection : graph.getConnections()) {
        auto* connElement = connectionsElement->createNewChildElement("CONNECTION");
        connElement->setAttribute("srcNode", (int)connection.source.nodeID.uid);
        connElement->setAttribute("srcCh", connection.source.channelIndex);
        connElement->setAttribute("dstNode", (int)connection.destination.nodeID.uid);
        connElement->setAttribute("dstCh", connection.destination.channelIndex);
        connElement->setAttribute("isMidi", connection.source.isMIDI());
    }

    xml.writeTo(file);
}

void GraphEditor::loadPreset(juce::File file) {
    auto xml = juce::XmlDocument::parse(file);
    if (!xml || !xml->hasTagName("GRAVISYNTH_PATCH")) {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Load Failed",
                                               "Could not load patch file.");
        return;
    }

    auto& graph = audioEngine.getGraph();
    graph.clear(); // Remove everything

    // 1. Nodes
    if (auto* nodesElement = xml->getChildByName("NODES")) {
        for (auto* nodeElement : nodesElement->getChildIterator()) {
            juce::String type = nodeElement->getStringAttribute("type");
            int x = nodeElement->getIntAttribute("x");
            int y = nodeElement->getIntAttribute("y");

            std::unique_ptr<juce::AudioProcessor> newProcessor;

            if (type == "Audio Input") {
                newProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                    juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode);
            } else if (type == "Audio Output") {
                newProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                    juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);
            } else if (type == "Midi Input") {
                newProcessor = std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                    juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode);
            } else if (type == "Oscillator") {
                newProcessor = std::make_unique<OscillatorModule>();
            } else if (type == "Filter") {
                newProcessor = std::make_unique<FilterModule>();
            } else if (type == "VCA") {
                newProcessor = std::make_unique<VCAModule>();
            } else if (type == "ADSR" || type.contains("Env")) {
                newProcessor = std::make_unique<ADSRModule>(type);
            } else if (type == "Sequencer") {
                newProcessor = std::make_unique<SequencerModule>();
            } else if (type == "LFO") {
                newProcessor = std::make_unique<LFOModule>();
            } else if (type == "Distortion") {
                newProcessor = std::make_unique<DistortionModule>();
            } else if (type == "Delay") {
                newProcessor = std::make_unique<DelayModule>();
            } else if (type == "Reverb") {
                newProcessor = std::make_unique<ReverbModule>();
            }

            if (newProcessor) {
                juce::MemoryBlock stateData;
                if (stateData.fromBase64Encoding(nodeElement->getStringAttribute("state"))) {
                    newProcessor->setStateInformation(stateData.getData(), (int)stateData.getSize());
                }

                auto node = graph.addNode(std::move(newProcessor));
                if (node) {
                    node->properties.set("x", x);
                    node->properties.set("y", y);
                    nodeElement->setAttribute("newId", (int)node->nodeID.uid);
                }
            }
        }
    }

    // 2. Connections
    if (auto* connElementBase = xml->getChildByName("CONNECTIONS")) {
        for (auto* connElement : connElementBase->getChildIterator()) {
            int oldSrcNodeId = connElement->getIntAttribute("srcNode");
            int srcCh = connElement->getIntAttribute("srcCh");
            int oldDstNodeId = connElement->getIntAttribute("dstNode");
            int dstCh = connElement->getIntAttribute("dstCh");
            bool isMidi = connElement->getBoolAttribute("isMidi");

            juce::AudioProcessorGraph::NodeID newSrcID, newDstID;

            if (auto* nodesElement = xml->getChildByName("NODES")) {
                for (auto* nodeElement : nodesElement->getChildIterator()) {
                    if (nodeElement->getIntAttribute("id") == oldSrcNodeId)
                        newSrcID =
                            juce::AudioProcessorGraph::NodeID((juce::uint32)nodeElement->getIntAttribute("newId"));
                    if (nodeElement->getIntAttribute("id") == oldDstNodeId)
                        newDstID =
                            juce::AudioProcessorGraph::NodeID((juce::uint32)nodeElement->getIntAttribute("newId"));
                }
            }

            if (newSrcID != juce::AudioProcessorGraph::NodeID() && newDstID != juce::AudioProcessorGraph::NodeID()) {
                graph.addConnection({{newSrcID, srcCh}, {newDstID, dstCh}});
            }
        }
    }

    updateComponents();
}
