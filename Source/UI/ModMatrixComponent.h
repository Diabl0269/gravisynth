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

    void setFlatSourceMenu(bool shouldBeFlat);

    void clearRows() {
        rows.clear();
        repaint();
    }

private:
    AudioEngine& audioEngine;
    bool isSourceMenuFlat = false;

    juce::TextButton addButton{"Add Modulation"};
    juce::ToggleButton flatToggle{"Flat Sources"};

    struct ModRow
        : public juce::Component
        , public juce::ComboBox::Listener {
        ModRow(ModMatrixComponent& owner, juce::AudioProcessorGraph::NodeID id);
        ~ModRow() override;

        void setRowIndex(int index) {
            rowIndex = index;
            repaint();
        }
        int rowIndex = 0;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void comboBoxChanged(juce::ComboBox* comboBox) override;

        ModMatrixComponent& owner;
        juce::AudioProcessorGraph::NodeID attenuverterId;

        juce::ComboBox sourceCombo;
        juce::ComboBox destCombo;
        juce::Slider amountSlider;
        juce::TextButton bypassToggle{"B"};
        juce::TextButton deleteButton{"X"};

        std::unique_ptr<juce::SliderParameterAttachment> amountAttachment;
        std::unique_ptr<juce::ButtonParameterAttachment> bypassAttachment;

        void refresh(const AudioEngine::ModRoutingInfo& info);
        void populateCombos();
    };

    std::vector<std::unique_ptr<ModRow>> rows;
    juce::Viewport viewport;
    juce::Component contentContainer;

    void updateRowsFromGraph();
    void addModulation();

    int lastNodeCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixComponent)
};
