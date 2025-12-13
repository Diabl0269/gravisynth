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
      auto *comp = new ModuleComponent(processor);
      addAndMakeVisible(comp);

      // Deterministic Layout
      // Deterministic Layout
      if (processor->getName() == "Sequencer")
        comp->setTopLeftPosition(10, 100); // Left-most, wide
      else if (processor->getName() == "Oscillator")
        comp->setTopLeftPosition(540, 100); // Right of Sequencer (510+padding)
      else if (processor->getName() == "ADSR")
        comp->setTopLeftPosition(540, 350); // Below Osc
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
        // We need to match module to node.
        // Since ModuleBase doesn't know its NodeID, and AudioProcessor doesn't
        // either easily without traversing graph... Let's rely on the pointer
        // address which is shared.

        auto *node1 = graph.getNodeForId(connection.source.nodeID);
        auto *node2 = graph.getNodeForId(connection.destination.nodeID);

        if (node1 && node1->getProcessor() == module) {
          p1 = comp->getBounds().getCentre();
          found1 = true;
        }
        if (node2 && node2->getProcessor() == module) {
          p2 = comp->getBounds().getCentre();
          found2 = true;
        }
      }
    }

    if (found1 && found2)
      g.drawLine(p1.toFloat().x, p1.toFloat().y, p2.toFloat().x, p2.toFloat().y,
                 2.0f);
  }

  g.setColour(juce::Colours::white);
  g.drawText("Gravisynth (Double click to refresh)",
             getLocalBounds().removeFromTop(20), juce::Justification::centred,
             true);
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
