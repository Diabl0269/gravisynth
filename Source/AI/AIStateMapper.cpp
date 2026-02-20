#include "AIStateMapper.h"
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
    {"ADSR", []() { return std::make_unique<ADSRModule>("ADSR"); }}, // ADSR constructor for generic case
    {"Sequencer", []() { return std::make_unique<SequencerModule>(); }},
    {"LFO", []() { return std::make_unique<LFOModule>(); }},
    {"Distortion", []() { return std::make_unique<DistortionModule>(); }},
    {"Delay", []() { return std::make_unique<DelayModule>(); }},
    {"Reverb", []() { return std::make_unique<ReverbModule>(); }},
    {"MIDI Keyboard", []() { return std::make_unique<MidiKeyboardModule>(); }}};

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

juce::String AIStateMapper::getModuleSchema() {
    juce::String schema = "### Available Modules and Parameters\n\n";

    for (const auto& entry : moduleFactory) {
        auto processor = entry.second();
        if (!processor)
            continue;

        schema += "#### " + entry.first + "\n";
        schema += "| Parameter ID | Name | Range / Options | Default |\n";
        schema += "| :--- | :--- | :--- | :--- |\n";

        for (auto* param : processor->getParameters()) {
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param)) {
                juce::String rangeStr;
                if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(param)) {
                    rangeStr = "Choice: [" + choice->choices.joinIntoString(", ") + "]";
                } else if (dynamic_cast<juce::AudioParameterBool*>(param)) {
                    rangeStr = "Boolean (0 or 1)";
                } else {
                    auto range = p->getNormalisableRange();
                    rangeStr = juce::String(range.start) + " to " + juce::String(range.end);
                }

                schema += "| `" + p->paramID + "` | " + p->name + " | " + rangeStr + " | " +
                          juce::String(p->getDefaultValue()) + " |\n";
            }
        }
        schema += "\n";
    }

    return schema;
}

int AIStateMapper::findChoiceIndex(juce::AudioParameterChoice* p, const juce::String& choiceText) {
    // 1. Exact match
    int index = p->choices.indexOf(choiceText);
    if (index >= 0)
        return index;

    // 2. Case-insensitive match
    for (int i = 0; i < p->choices.size(); ++i) {
        if (p->choices[i].equalsIgnoreCase(choiceText))
            return i;
    }

    return -1;
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
                                    if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param)) {
                                        if (pObj->hasProperty(p->paramID)) {
                                            auto jsonValue = pObj->getProperty(p->paramID);

                                            if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(p)) {
                                                if (jsonValue.isString()) {
                                                    int index = findChoiceIndex(choice, jsonValue.toString());
                                                    if (index >= 0) {
                                                        p->setValueNotifyingHost(
                                                            p->getNormalisableRange().convertTo0to1((float)index));
                                                    }
                                                } else {
                                                    float val = (float)jsonValue;
                                                    p->setValueNotifyingHost(
                                                        p->getNormalisableRange().convertTo0to1(val));
                                                }
                                            } else if (auto* b = dynamic_cast<juce::AudioParameterBool*>(p)) {
                                                b->setValueNotifyingHost((bool)jsonValue ? 1.0f : 0.0f);
                                            } else {
                                                float val = (float)jsonValue;
                                                // Convert the value from the JSON (unnormalized) to the parameter's
                                                // normalized range
                                                val = p->getNormalisableRange().snapToLegalValue(
                                                    val); // Snap to legal value first
                                                float normalizedValue = p->getNormalisableRange().convertTo0to1(val);
                                                p->setValueNotifyingHost(normalizedValue);
                                            }
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

juce::var AIStateMapper::getPatchSchema() {
    juce::DynamicObject::Ptr schema = new juce::DynamicObject();
    schema->setProperty("type", "object");

    juce::DynamicObject::Ptr properties = new juce::DynamicObject();

    // 1. Nodes
    juce::DynamicObject::Ptr nodes = new juce::DynamicObject();
    nodes->setProperty("type", "array");
    juce::DynamicObject::Ptr nodeItems = new juce::DynamicObject();
    nodeItems->setProperty("type", "object");
    juce::DynamicObject::Ptr nodeProperties = new juce::DynamicObject();

    nodeProperties->setProperty("id", juce::JSON::parse("{\"type\": \"integer\"}"));
    nodeProperties->setProperty(
        "type", juce::JSON::parse("{\"type\": \"string\", \"enum\": [\"Audio Input\", \"Audio Output\", \"Midi "
                                  "Input\", \"Oscillator\", \"Filter\", \"VCA\", \"ADSR\", \"Sequencer\", \"LFO\", "
                                  "\"Distortion\", \"Delay\", \"Reverb\", \"MIDI Keyboard\"]}"));
    nodeProperties->setProperty("params", juce::JSON::parse("{\"type\": \"object\"}"));

    nodeItems->setProperty("properties", juce::var(nodeProperties.get()));
    nodeItems->setProperty("required", juce::Array<juce::var>({"id", "type"}));
    nodes->setProperty("items", juce::var(nodeItems.get()));
    properties->setProperty("nodes", juce::var(nodes.get()));

    // 2. Connections
    juce::DynamicObject::Ptr connections = new juce::DynamicObject();
    connections->setProperty("type", "array");
    juce::DynamicObject::Ptr connItems = new juce::DynamicObject();
    connItems->setProperty("type", "object");
    juce::DynamicObject::Ptr connProperties = new juce::DynamicObject();

    connProperties->setProperty("src", juce::JSON::parse("{\"type\": \"integer\"}"));
    connProperties->setProperty("srcPort", juce::JSON::parse("{\"type\": \"integer\"}"));
    connProperties->setProperty("dst", juce::JSON::parse("{\"type\": \"integer\"}"));
    connProperties->setProperty("dstPort", juce::JSON::parse("{\"type\": \"integer\"}"));

    connItems->setProperty("properties", juce::var(connProperties.get()));
    connItems->setProperty("required", juce::Array<juce::var>({"src", "srcPort", "dst", "dstPort"}));
    connections->setProperty("items", juce::var(connItems.get()));
    properties->setProperty("connections", juce::var(connections.get()));

    schema->setProperty("properties", juce::var(properties.get()));
    schema->setProperty("required", juce::Array<juce::var>({"nodes", "connections"}));

    return juce::var(schema.get());
}

} // namespace gsynth
