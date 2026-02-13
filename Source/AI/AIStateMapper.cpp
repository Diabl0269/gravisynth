#include "AIStateMapper.h"
#include "../Modules/ADSRModule.h"
#include "../Modules/FX/DelayModule.h"
#include "../Modules/FX/DistortionModule.h"
#include "../Modules/FX/ReverbModule.h"
#include "../Modules/FilterModule.h"
#include "../Modules/LFOModule.h"
#include "../Modules/OscillatorModule.h"
#include "../Modules/SequencerModule.h"
#include "../Modules/VCAModule.h"
#include <map>

namespace gsynth {

juce::var AIStateMapper::graphToJSON(juce::AudioProcessorGraph& graph) {
    juce::DynamicObject::Ptr root = new juce::DynamicObject();

    juce::Array<juce::var> nodes;
    for (auto* node : graph.getNodes()) {
        if (auto* processor = node->getProcessor()) {
            juce::DynamicObject::Ptr n = new juce::DynamicObject();
            n->setProperty("id", (int)node->nodeID.uid);
            n->setProperty("type", processor->getName());

            // Params
            juce::DynamicObject::Ptr params = new juce::DynamicObject();
            for (auto* param : processor->getParameters()) {
                if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
                    params->setProperty(p->paramID, p->getValue());
                }
            }
            n->setProperty("params", juce::var(params.get()));

            // Position
            juce::DynamicObject::Ptr pos = new juce::DynamicObject();
            pos->setProperty("x", node->properties["x"]);
            pos->setProperty("y", node->properties["y"]);
            n->setProperty("position", juce::var(pos.get()));

            nodes.add(juce::var(n.get()));
        }
    }
    root->setProperty("nodes", nodes);

    juce::Array<juce::var> connections;
    for (const auto& conn : graph.getConnections()) {
        juce::DynamicObject::Ptr c = new juce::DynamicObject();
        c->setProperty("src", (int)conn.source.nodeID.uid);
        c->setProperty("srcPort", conn.source.channelIndex);
        c->setProperty("dst", (int)conn.destination.nodeID.uid);
        c->setProperty("dstPort", conn.destination.channelIndex);
        c->setProperty("isMidi", conn.source.isMIDI());
        connections.add(juce::var(c.get()));
    }
    root->setProperty("connections", connections);

    return juce::var(root.get());
}

bool AIStateMapper::applyJSONToGraph(const juce::var& json, juce::AudioProcessorGraph& graph) {
    if (!json.isObject())
        return false;
    auto* rootObj = json.getDynamicObject();
    if (!rootObj)
        return false;

    graph.clear(); // For now, we rebuild. Future: partial updates.

    std::map<int, juce::AudioProcessorGraph::NodeID> idMap;

    // 1. Create Nodes
    if (rootObj->hasProperty("nodes")) {
        auto* nodesList = rootObj->getProperty("nodes").getArray();
        if (nodesList) {
            for (const auto& nVar : *nodesList) {
                if (auto* nObj = nVar.getDynamicObject()) {
                    int oldId = nObj->getProperty("id");
                    juce::String type = nObj->getProperty("type");

                    auto processor = createModule(type);
                    if (processor) {
                        // Set parameters
                        if (nObj->hasProperty("params")) {
                            if (auto* pObj = nObj->getProperty("params").getDynamicObject()) {
                                for (auto* param : processor->getParameters()) {
                                    if (auto* p = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
                                        if (pObj->hasProperty(p->paramID)) {
                                            p->setValue((float)pObj->getProperty(p->paramID));
                                        }
                                    }
                                }
                            }
                        }

                        auto node = graph.addNode(std::move(processor));
                        if (node) {
                            idMap[oldId] = node->nodeID;
                            if (nObj->hasProperty("position")) {
                                if (auto* posObj = nObj->getProperty("position").getDynamicObject()) {
                                    node->properties.set("x", posObj->getProperty("x"));
                                    node->properties.set("y", posObj->getProperty("y"));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 2. Connections
    if (rootObj->hasProperty("connections")) {
        auto* connList = rootObj->getProperty("connections").getArray();
        if (connList) {
            for (const auto& cVar : *connList) {
                if (auto* cObj = cVar.getDynamicObject()) {
                    int srcOld = cObj->getProperty("src");
                    int dstOld = cObj->getProperty("dst");
                    int srcPort = cObj->getProperty("srcPort");
                    int dstPort = cObj->getProperty("dstPort");

                    if (idMap.count(srcOld) && idMap.count(dstOld)) {
                        graph.addConnection({{idMap[srcOld], srcPort}, {idMap[dstOld], dstPort}});
                    }
                }
            }
        }
    }

    return true;
}

std::unique_ptr<juce::AudioProcessor> AIStateMapper::createModule(const juce::String& type) {
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

    if (type == "Audio Input")
        return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode);
    if (type == "Audio Output")
        return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode);
    if (type == "Midi Input")
        return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::midiInputNode);

    if (type == "Oscillator")
        return std::make_unique<OscillatorModule>();
    if (type == "Filter")
        return std::make_unique<FilterModule>();
    if (type == "VCA")
        return std::make_unique<VCAModule>();
    if (type == "ADSR" || type.contains("Env"))
        return std::make_unique<ADSRModule>(type);
    if (type == "Sequencer")
        return std::make_unique<SequencerModule>();
    if (type == "LFO")
        return std::make_unique<LFOModule>();
    if (type == "Distortion")
        return std::make_unique<DistortionModule>();
    if (type == "Delay")
        return std::make_unique<DelayModule>();
    if (type == "Reverb")
        return std::make_unique<ReverbModule>();

    return nullptr;
}

} // namespace gsynth
