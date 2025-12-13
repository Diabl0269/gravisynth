#include "GraphEditor.h"
#include "ModuleComponent.h"

GraphEditor::GraphEditor(AudioEngine &engine) : audioEngine(engine) {
  updateComponents();
  // startTimer(500); // Check for graph changes periodically (naive approach)
}

GraphEditor::~GraphEditor() {}

void GraphEditor::updateComponents() {
  // Naive sync: clear and recreate. In production we'd diff.
  removeAllChildren();
  moduleComponents.clear();

  auto &graph = audioEngine.getGraph();
  for (auto *node : graph.getNodes()) {
    if (auto *processor = node->getProcessor()) {
      // Create component for ANY processor (ModuleBase OR IO)
      auto *comp = new ModuleComponent(processor, *this);
      addAndMakeVisible(comp);

      // Deterministic Layout
      // Deterministic Layout
      if (processor->getName() == "Sequencer")
        comp->setTopLeftPosition(10, 100); // Left-most, wide
      else if (processor->getName() == "Oscillator")
        comp->setTopLeftPosition(540, 100); // Right of Sequencer (510+padding)
      else if (processor->getName() == "Amp Env")
        comp->setTopLeftPosition(1000, 350); // Below VCA/Filter area
      else if (processor->getName() == "Filter Env")
        comp->setTopLeftPosition(760, 350); // Below Filter
      else if (processor->getName() == "ADSR")
        comp->setTopLeftPosition(540, 350); // Fallback
      else if (processor->getName() == "Filter")
        comp->setTopLeftPosition(760, 100); // Right of Osc
      else if (processor->getName() == "VCA")
        comp->setTopLeftPosition(980, 100); // Right of Filter (Linear flow)
      else if (processor->getName() == "Audio Output")
        comp->setTopLeftPosition(1200, 100); // Far right
      else if (processor->getName() == "Audio Input")
        comp->setTopLeftPosition(10, 10);
      else
        comp->setTopLeftPosition(100 + (moduleComponents.size() * 20), 500);

      moduleComponents.add(comp);
    }
  }
}

void GraphEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::darkgrey);

  // Draw connections
  auto &graph = audioEngine.getGraph();
  g.setColour(juce::Colours::yellow);

  for (auto &connection : graph.getConnections()) {
    juce::Point<int> p1, p2;
    bool found1 = false;
    bool found2 = false;

    // Naive search for components matching the nodes
    for (auto *comp : moduleComponents) {
      if (auto *module = comp->getModule()) {

        auto *node1 = graph.getNodeForId(connection.source.nodeID);
        auto *node2 = graph.getNodeForId(connection.destination.nodeID);

        if (node1 && node1->getProcessor() == module) {
          // connection.source is an Output from node1
          juce::Point<int> localP;
          if (connection.source.channelIndex ==
              juce::AudioProcessorGraph::midiChannelIndex) {
            localP = comp->getPortCenter(0, false); // Sequencer Midi Out
          } else {
            localP = comp->getPortCenter(connection.source.channelIndex, false);
          }
          p1 = comp->getBounds().getPosition() + localP;
          found1 = true;
        }
        if (node2 && node2->getProcessor() == module) {
          // connection.destination is an Input to node2
          juce::Point<int> localP;
          if (connection.destination.channelIndex ==
              juce::AudioProcessorGraph::midiChannelIndex) {
            localP = juce::Point<int>(10, 30); // Midi In
          } else {
            localP =
                comp->getPortCenter(connection.destination.channelIndex, true);
          }
          p2 = comp->getBounds().getPosition() + localP;
          found2 = true;
        }
      }
    }

    if (found1 && found2)
      g.drawLine(p1.toFloat().x, p1.toFloat().y, p2.toFloat().x, p2.toFloat().y,
                 2.0f);
  }

  // Draw Line being dragged
  if (isDraggingConnection) {
    g.setColour(juce::Colours::white);

    if (dragSourceModule) {
      juce::Point<int> p;
      if (dragSourceIsMidi) {
        // Midi port pos hardcoded or dynamic?
        // DragSourceModule logic for Midi Out (Sequencer) is at port 0, false.
        // DragSourceModule logic for Midi In (Osc) is at 10, 30.
        if (dragSourceIsInput)
          p = juce::Point<int>(10, 30); // In
        else
          p = dragSourceModule->getPortCenter(0, false); // Out
      } else {
        p = dragSourceModule->getPortCenter(dragSourceChannel,
                                            dragSourceIsInput);
      }

      auto screenP = dragSourceModule->localPointToGlobal(p);
      auto localP = getLocalPoint(nullptr, screenP);
      auto mouseLocal = getLocalPoint(nullptr, dragCurrentPos);
      g.drawLine(localP.toFloat().x, localP.toFloat().y, mouseLocal.toFloat().x,
                 mouseLocal.toFloat().y, 3.0f);
    }
  }

  g.setColour(juce::Colours::white);
  g.drawText("Gravisynth (Double click to refresh)",
             getLocalBounds().removeFromTop(20), juce::Justification::centred,
             true);
}

void GraphEditor::beginConnectionDrag(ModuleComponent *sourceModule,
                                      int channelIndex, bool isInput,
                                      bool isMidi, juce::Point<int> screenPos) {
  isDraggingConnection = true;
  dragSourceModule = sourceModule;
  dragSourceChannel = channelIndex;
  dragSourceIsInput = isInput;
  dragSourceIsMidi = isMidi;
  dragCurrentPos = screenPos;
  repaint();
}

void GraphEditor::dragConnection(juce::Point<int> screenPos) {
  if (!isDraggingConnection)
    return;
  dragCurrentPos = screenPos;
  repaint();
}

void GraphEditor::endConnectionDrag(juce::Point<int> screenPos) {
  if (!isDraggingConnection)
    return;

  // Hit test for target
  // We need to find which module/port is under screenPos.
  // Iterate modules.

  for (auto *comp : moduleComponents) {
    auto localPos = comp->getLocalPoint(nullptr, screenPos);
    auto port = comp->getPortForPoint(localPos);

    if (port) {
      // Valid drop?
      // Prevent same module
      if (comp == dragSourceModule)
        continue;

      // Prevent Input->Input or Output->Output
      if (port->isInput == dragSourceIsInput)
        continue;

      // Prevent Mismatched Types (Audio->Midi or Midi->Audio)
      if (port->isMidi != dragSourceIsMidi)
        continue;

      // Connect!
      // Source is DragSource, Dest is Comp (or vice versa)
      juce::AudioProcessorGraph::NodeID srcNodeId, dstNodeId;
      int srcCh, dstCh;

      // Graph needs NodeIDs.
      // We need to find NodeIDs via Processor pointer match.
      // Helper needed.
      auto &graph = audioEngine.getGraph();
      juce::AudioProcessorGraph::Node *srcNode = nullptr;
      juce::AudioProcessorGraph::Node *dstNode = nullptr;

      for (auto *n : graph.getNodes()) {
        if (n->getProcessor() == dragSourceModule->getModule())
          srcNode = n;
        if (n->getProcessor() == comp->getModule())
          dstNode = n;
      }

      if (srcNode && dstNode) {

        if (dragSourceIsMidi) {
          // MIDI Connection
          // Direction: Output(Src) -> Input(Dst) always?
          // Sequencer (Out) -> Oscillator (In)
          // If dragging from IN (if we allowed Midi In dragging? No), logic
          // follows.

          // Assuming standard flow: Out -> In.
          // Midi Channel Indices in JUCE Graph are special constants usually.
          // But addConnection just wants channel indices.
          // wait, AudioProcessorGraph::midiChannelIndex is the constant.

          graph.addConnection(
              {{srcNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
               {dstNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
        } else {
          // AUDIO Connection
          if (dragSourceIsInput) {
            // Dragged from Input -> Dropped on Output
            graph.addConnection({{dstNode->nodeID, port->index},
                                 {srcNode->nodeID, dragSourceChannel}});
          } else {
            // Dragged from Output -> Dropped on Input
            graph.addConnection({{srcNode->nodeID, dragSourceChannel},
                                 {dstNode->nodeID, port->index}});
          }
        }
      }
    }
  }

  isDraggingConnection = false;
  dragSourceModule = nullptr;
  repaint();
}

void GraphEditor::resized() {}

void GraphEditor::timerCallback() {
  // updateComponents(); // CAUSES FLICKER/RESET
  // TODO: Smart diffing
}

void GraphEditor::mouseDoubleClick(const juce::MouseEvent &) {
  updateComponents();
  repaint();
}

void GraphEditor::disconnectPort(ModuleComponent *module, int portIndex,
                                 bool isInput, bool isMidi) {
  auto &graph = audioEngine.getGraph();
  juce::AudioProcessorGraph::NodeID nodeId;

  // Find NodeID
  for (auto *n : graph.getNodes()) {
    if (n->getProcessor() == module->getModule()) {
      nodeId = n->nodeID;
      break;
    }
  }

  if (nodeId.uid == 0)
    return; // Not found

  // Collect connections to remove (can't remove while iterating)
  std::vector<juce::AudioProcessorGraph::Connection> toRemove;

  int targetChannel =
      isMidi ? juce::AudioProcessorGraph::midiChannelIndex : portIndex;

  for (auto &c : graph.getConnections()) {
    if (isInput) {
      // Disconnect INPUT: Check destination
      if (c.destination.nodeID == nodeId &&
          c.destination.channelIndex == targetChannel) {
        toRemove.push_back(c);
      }
    } else {
      // Disconnect OUTPUT: Check source
      if (c.source.nodeID == nodeId && c.source.channelIndex == targetChannel) {
        toRemove.push_back(c);
      }
    }
  }

  for (auto &c : toRemove) {
    graph.removeConnection(c);
  }

  repaint();
}
