#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

/**
 * @brief Abstract interface for graph management operations.
 *
 * Decouples UI components (GraphEditor, ModMatrixComponent, ModuleComponent)
 * from the concrete AudioEngine class, allowing both the standalone app
 * and the VST3/AU plugin to share the same UI code.
 */
class IGraphManager {
public:
    virtual ~IGraphManager() = default;

    struct ModRoutingInfo {
        juce::AudioProcessorGraph::NodeID attenuverterNodeID;
        juce::AudioProcessorGraph::NodeID sourceNodeID;
        int sourceChannelIndex = 0;
        juce::AudioProcessorGraph::NodeID destNodeID;
        int destChannelIndex = 0;
        bool isBypassed = false;
    };

    virtual juce::AudioProcessorGraph& getGraph() = 0;

    virtual std::vector<ModRoutingInfo> getActiveModRoutings() const = 0;
    virtual void addModRouting(juce::AudioProcessorGraph::NodeID sourceNodeID, int sourceChannelIndex,
                               juce::AudioProcessorGraph::NodeID destNodeID, int destChannelIndex) = 0;
    virtual void addEmptyModRouting() = 0;
    virtual void removeModRouting(juce::AudioProcessorGraph::NodeID attenuverterNodeID) = 0;
    virtual void toggleModBypass(juce::AudioProcessorGraph::NodeID attenuverterNodeID) = 0;
    virtual bool isModBypassed(juce::AudioProcessorGraph::NodeID attenuverterNodeID) const = 0;
    virtual void updateModuleNames() = 0;
};
