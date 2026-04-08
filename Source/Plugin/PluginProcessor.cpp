#include "PluginProcessor.h"
#include "../AI/AIStateMapper.h"
#include "../PresetManager.h"
#include "../Modules/AttenuverterModule.h"
#include "../Modules/ModuleBase.h"
#include "PluginEditor.h"
#include <map>

GravisynthPluginProcessor::GravisynthPluginProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
    if (!gsynth::PresetManager::loadDefaultPreset(processorGraph)) {
        createDefaultPatch();
    }
    ensureMidiInputNode();
}

GravisynthPluginProcessor::~GravisynthPluginProcessor() {
    processorGraph.clear();
}

void GravisynthPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    processorGraph.setPlayConfigDetails(0, 2, sampleRate, samplesPerBlock);
    processorGraph.prepareToPlay(sampleRate, samplesPerBlock);
}

void GravisynthPluginProcessor::releaseResources() { processorGraph.releaseResources(); }

void GravisynthPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    processorGraph.processBlock(buffer, midiMessages);
}

juce::AudioProcessorEditor* GravisynthPluginProcessor::createEditor() { return new GravisynthPluginEditor(*this); }

void GravisynthPluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto json = gsynth::AIStateMapper::graphToJSON(processorGraph);
    auto jsonString = juce::JSON::toString(json);
    destData.append(jsonString.toRawUTF8(), jsonString.getNumBytesAsUTF8());
}

void GravisynthPluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto jsonString = juce::String::fromUTF8(static_cast<const char*>(data), sizeInBytes);
    auto json = juce::JSON::parse(jsonString);
    if (json.isObject()) {
        gsynth::AIStateMapper::applyJSONToGraph(json, processorGraph, true);
        ensureMidiInputNode();
    }
}

// --- IGraphManager implementation ---

std::vector<IGraphManager::ModRoutingInfo> GravisynthPluginProcessor::getActiveModRoutings() const {
    std::vector<IGraphManager::ModRoutingInfo> routings;
    for (auto* node : processorGraph.getNodes()) {
        if (dynamic_cast<AttenuverterModule*>(node->getProcessor()) != nullptr) {
            IGraphManager::ModRoutingInfo info;
            info.attenuverterNodeID = node->nodeID;
            info.sourceChannelIndex = 0;
            for (auto& conn : processorGraph.getConnections()) {
                if (conn.destination.nodeID == node->nodeID && conn.destination.channelIndex == 0) {
                    info.sourceNodeID = conn.source.nodeID;
                    info.sourceChannelIndex = conn.source.channelIndex;
                    break;
                }
            }
            for (auto& conn : processorGraph.getConnections()) {
                if (conn.source.nodeID == node->nodeID && conn.source.channelIndex == 0) {
                    info.destNodeID = conn.destination.nodeID;
                    info.destChannelIndex = conn.destination.channelIndex;
                    break;
                }
            }
            info.isBypassed = false;
            if (auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(node->getProcessor()->getParameters()[1])) {
                info.isBypassed = bypassParam->get();
            }
            routings.push_back(info);
        }
    }
    return routings;
}

void GravisynthPluginProcessor::addModRouting(juce::AudioProcessorGraph::NodeID sourceNodeID, int sourceChannelIndex,
                                              juce::AudioProcessorGraph::NodeID destNodeID, int destChannelIndex) {
    auto attenuverterNode = processorGraph.addNode(std::make_unique<AttenuverterModule>());
    if (attenuverterNode == nullptr)
        return;
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(attenuverterNode->getProcessor()->getParameters()[0]))
        param->setValueNotifyingHost(param->convertTo0to1(1.0f));
    processorGraph.addConnection({{sourceNodeID, sourceChannelIndex}, {attenuverterNode->nodeID, 0}});
    processorGraph.addConnection({{attenuverterNode->nodeID, 0}, {destNodeID, destChannelIndex}});
}

void GravisynthPluginProcessor::addEmptyModRouting() { processorGraph.addNode(std::make_unique<AttenuverterModule>()); }

void GravisynthPluginProcessor::removeModRouting(juce::AudioProcessorGraph::NodeID attenuverterNodeID) {
    processorGraph.removeNode(attenuverterNodeID);
}

void GravisynthPluginProcessor::toggleModBypass(juce::AudioProcessorGraph::NodeID attenuverterNodeID) {
    if (auto* node = processorGraph.getNodeForId(attenuverterNodeID)) {
        if (auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(node->getProcessor()->getParameters()[1])) {
            bypassParam->setValueNotifyingHost(!bypassParam->get());
        }
    }
}

bool GravisynthPluginProcessor::isModBypassed(juce::AudioProcessorGraph::NodeID attenuverterNodeID) const {
    if (auto* node = processorGraph.getNodeForId(attenuverterNodeID)) {
        if (auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(node->getProcessor()->getParameters()[1])) {
            return bypassParam->get();
        }
    }
    return false;
}

void GravisynthPluginProcessor::updateModuleNames() {
    std::map<juce::String, int> typeCounts;
    for (auto* node : processorGraph.getNodes()) {
        if (auto* module = dynamic_cast<ModuleBase*>(node->getProcessor())) {
            juce::String baseName = module->getName();
            int lastSpace = baseName.lastIndexOf(" ");
            if (lastSpace != -1 && baseName.substring(lastSpace + 1).containsOnly("0123456789"))
                baseName = baseName.substring(0, lastSpace);
            if (baseName.startsWith("Attenuverter"))
                baseName = "Mod Slot";
            int index = ++typeCounts[baseName];
            module->setModuleName(baseName + " " + juce::String(index));
        }
    }
}

void GravisynthPluginProcessor::ensureMidiInputNode() {
    bool hasMidiInput = false;
    for (auto* node : processorGraph.getNodes()) {
        if (auto* iop = dynamic_cast<juce::AudioProcessorGraph::AudioGraphIOProcessor*>(node->getProcessor())) {
            if (iop->getType() == juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode) {
                hasMidiInput = true;
                break;
            }
        }
    }
    if (!hasMidiInput) {
        auto midiInputNode = processorGraph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
            juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));
        if (midiInputNode) {
            for (auto* node : processorGraph.getNodes()) {
                if (node != midiInputNode.get() && node->getProcessor()->acceptsMidi()) {
                    processorGraph.addConnection({{midiInputNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex},
                                                  {node->nodeID, juce::AudioProcessorGraph::midiChannelIndex}});
                }
            }
        }
    }
}

void GravisynthPluginProcessor::createDefaultPatch() {
    processorGraph.clear();
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    processorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode));
    processorGraph.addNode(std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GravisynthPluginProcessor(); }
