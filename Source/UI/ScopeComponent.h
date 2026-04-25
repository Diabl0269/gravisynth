#pragma once

#include "../Modules/VisualBuffer.h"
#include <juce_gui_basics/juce_gui_basics.h>

class ScopeComponent
    : public juce::Component
    , public juce::Timer {
public:
    ScopeComponent(VisualBuffer& buffer)
        : visualBuffer(buffer) {
        sampleData.resize(buffer.getSize(), 0.0f);
        startTimerHz(60); // higher refresh rate for scope
    }

    ~ScopeComponent() override { stopTimer(); }

    void timerCallback() override {
        visualBuffer.copyTo(sampleData);
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);

        auto bounds = getLocalBounds().toFloat();
        auto midY = bounds.getCentreY();
        auto height = bounds.getHeight();
        auto width = bounds.getWidth();

        g.setColour(juce::Colours::limegreen);
        juce::Path p;

        float peak = 0.01f;
        for (float s : sampleData) {
            peak = std::max(peak, std::abs(s));
        }
        // Auto-scale to fit within 90% of the height, but never scale up more than a 1.0 amplitude signal
        float dynamicScale = std::min(1.0f, 1.0f / peak);

        bool first = true;
        for (int i = 0; i < (int)sampleData.size(); ++i) {
            float x = juce::jmap((float)i, 0.0f, (float)sampleData.size(), 0.0f, width);
            float y = midY - (sampleData[i] * dynamicScale * height * 0.45f);

            if (first) {
                p.startNewSubPath(x, y);
                first = false;
            } else {
                p.lineTo(x, y);
            }
        }

        g.strokePath(p, juce::PathStrokeType(1.5f));
    }

private:
    VisualBuffer& visualBuffer;
    std::vector<float> sampleData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeComponent)
};
