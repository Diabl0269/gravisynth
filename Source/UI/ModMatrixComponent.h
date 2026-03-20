#pragma once

#include "../AudioEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ModMatrixComponent
    : public juce::Component
    , public juce::Timer {
public:
    ModMatrixComponent(AudioEngine& engine);
    ~ModMatrixComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void clearRows() {
        rows.clear();
        repaint();
    }

private:
    AudioEngine& audioEngine;

    struct ModRow {
        juce::AudioProcessorGraph::NodeID nodeId;
        juce::String sourceName;
        juce::String destName;
        std::unique_ptr<juce::Slider> amountSlider;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
    };

    std::vector<std::unique_ptr<ModRow>> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixComponent)
};
