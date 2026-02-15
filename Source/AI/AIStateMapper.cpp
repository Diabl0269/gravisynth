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
#include <functional> // For std::function
#include <map>
#include <unordered_map> // For the factory map

namespace gsynth {

typedef juce::AudioProcessorGraph::AudioGraphIOProcessor AudioGraphIOProcessor;

using ModuleFactoryFunc = std::function<std::unique_ptr<juce::AudioProcessor>()>;

// Factory map for module creation
static const std::unordered_map<juce::String, ModuleFactoryFunc> moduleFactory = {
    {"Audio Input", []() { return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode); }},
    {"Audio Output", []() { return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode); }},
    {"Midi Input", []() { return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::midiInputNode); }},
    {"Oscillator", []() { return std::make_unique<OscillatorModule>(); }},
    {"Filter", []() { return std::make_unique<FilterModule>(); }},
    {"VCA", []() { return std::make_unique<VCAModule>(); }},
    {"ADSR", []() { return std::make_unique<ADSRModule>(); }}, // ADSR constructor for generic case
    {"Sequencer", []() { return std::make_unique<SequencerModule>(); }},
    {"LFO", []() { return std::make_unique<LFOModule>(); }},
    {"Distortion", []() { return std::make_unique<DistortionModule>(); }},
    {"Delay", []() { return std::make_unique<DelayModule>(); }},
    {"Reverb", []() { return std::make_unique<ReverbModule>(); }}};

bool AIStateMapper::validatePatchJSON(const juce::var& json) {
    if (!json.isObject()) {
        juce::Logger::writeToLog("validatePatchJSON: Root is not an object.");
        return false;
    }
    auto* rootObj = json.getDynamicObject();
    if (!rootObj) {
        juce::Logger::writeToLog("validatePatchJSON: Root dynamic object is null.");
        return false;
    }

    // Check for "nodes" array
    if (rootObj->hasProperty("nodes")) {
        auto* nodesList = rootObj->getProperty("nodes").getArray();
        if (nodesList == nullptr) {
            juce::Logger::writeToLog("validatePatchJSON: 'nodes' property is not an array.");
            return false;
        }
        // Further validation of each node could go here (e.g., checking for id and type)
    } else {
        juce::Logger::writeToLog("validatePatchJSON: 'nodes' property is missing.");
        return false;
    }

    // Check for "connections" array (optional, a patch can exist without connections)
    if (rootObj->hasProperty("connections")) {
        auto* connList = rootObj->getProperty("connections").getArray();
        if (connList == nullptr) {
            juce::Logger::writeToLog("validatePatchJSON: 'connections' property is not an array.");
            return false;
        }
        // Further validation of each connection could go here
    }

    return true;
}

std::unique_ptr<juce::AudioProcessor> AIStateMapper::createModule(const juce::String& type) {
    auto it = moduleFactory.find(type);
    if (it != moduleFactory.end()) {
        return it->second();
    }
    juce::Logger::writeToLog("AIStateMapper: Unknown module type: " + type);
    return nullptr;
}

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

bool AIStateMapper::applyJSONToGraph(const juce::var& json, juce::AudioProcessorGraph& graph, bool clearExisting) {
    if (!json.isObject()) {
        juce::Logger::writeToLog("applyJSONToGraph: JSON is not an object.");
        return false;
    }
    auto* rootObj = json.getDynamicObject();
    if (!rootObj) {
        juce::Logger::writeToLog("applyJSONToGraph: JSON dynamic object is null.");
        return false;
    }

    // Validate JSON structure before making any changes
    if (!validatePatchJSON(json)) {
        juce::Logger::writeToLog("applyJSONToGraph: JSON patch validation failed.");
        return false;
    }

    const juce::ScopedLock sl(graph.getCallbackLock());

    if (clearExisting) {
        graph.clear(); // Clear existing graph as requested
    }

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
                                            float value = (float)pObj->getProperty(p->paramID);
                                            // Clamp value to the parameter's normalized range
                                            value = juce::jlimit(0.0f, 1.0f, value);
                                            p->setValue(value);
                                        } else {
                                            juce::Logger::writeToLog("AIStateMapper: Parameter '" + p->paramID +
                                                                     "' not found in JSON for module '" + type + "'");
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

} // namespace gsynth
