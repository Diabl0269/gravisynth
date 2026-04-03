#pragma once

#include "../Modules/FilterModule.h"
#include "../Modules/VisualBuffer.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class FrequencyResponseComponent
    : public juce::Component
    , public juce::Timer {
public:
    FrequencyResponseComponent(FilterModule& filter)
        : filterModule(filter) {
        magnitudes.resize(numPoints, 0.0f);
        fftData.resize(fftSize * 2, 0.0f);
        spectrumMagnitudes.resize(numPoints, -80.0f);
        startTimerHz(30);
    }

    ~FrequencyResponseComponent() override { stopTimer(); }

    void setShowSpectrum(bool show) { showSpectrum = show; repaint(); }
    bool getShowSpectrum() const { return showSpectrum; }

    void timerCallback() override {
        float cutoff = filterModule.getCurrentCutoff();
        float resonance = filterModule.getCurrentResonance();
        float drive = filterModule.getCurrentDrive();
        int filterType = filterModule.getCurrentFilterType();

        if (cutoff != lastCutoff || resonance != lastResonance ||
            drive != lastDrive || filterType != lastFilterType) {
            lastCutoff = cutoff;
            lastResonance = resonance;
            lastDrive = drive;
            lastFilterType = filterType;
            recomputeMagnitudes();
            repaint();
        }

        // Update spectrum from audio data
        if (showSpectrum && filterModule.getVisualBuffer()) {
            auto* vb = filterModule.getVisualBuffer();
            std::vector<float> samples(vb->getSize(), 0.0f);
            vb->copyTo(samples);

            // Fill FFT buffer (zero-pad if needed)
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            int copySize = std::min((int)samples.size(), fftSize);
            for (int i = 0; i < copySize; ++i)
                fftData[i] = samples[i];

            // Apply window and perform FFT
            window.multiplyWithWindowingTable(fftData.data(), fftSize);
            fft.performFrequencyOnlyForwardTransform(fftData.data());

            // Convert to dB at log-spaced frequency points
            double sampleRate = filterModule.getLastSampleRate();
            float binWidth = static_cast<float>(sampleRate) / static_cast<float>(fftSize);
            float normFactor = 2.0f / static_cast<float>(fftSize);

            for (int i = 0; i < numPoints; ++i) {
                float freq = indexToFreq(i);
                float exactBin = freq / binWidth;
                int bin0 = static_cast<int>(exactBin);
                int bin1 = bin0 + 1;
                float frac = exactBin - static_cast<float>(bin0);

                float mag = 0.0f;
                if (bin0 >= 0 && bin1 < fftSize / 2) {
                    mag = fftData[bin0] * (1.0f - frac) + fftData[bin1] * frac;
                } else if (bin0 >= 0 && bin0 < fftSize / 2) {
                    mag = fftData[bin0];
                }
                mag *= normFactor;

                float db = 20.0f * std::log10(std::max(mag, 0.00001f));
                float target = juce::jlimit(-80.0f, 0.0f, db);
                // Smooth the spectrum (exponential moving average)
                spectrumMagnitudes[i] += 0.3f * (target - spectrumMagnitudes[i]);
            }

            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        float w = bounds.getWidth();
        float h = bounds.getHeight();

        // Dark background
        g.fillAll(juce::Colour(0xff1a1a2e));

        // Grid lines at 100Hz, 1kHz, 10kHz
        g.setColour(juce::Colour(0xff2a2a3e));
        for (float freq : {100.0f, 1000.0f, 10000.0f}) {
            float x = freqToX(freq, w);
            g.drawVerticalLine((int)x, 0.0f, h);
        }
        // 0dB horizontal line
        float zeroDbY = dbToY(0.0f, h);
        g.drawHorizontalLine((int)zeroDbY, 0.0f, w);

        // Build the path from magnitude data
        juce::Path curvePath;
        juce::Path fillPath;

        bool first = true;
        for (int i = 0; i < numPoints; ++i) {
            float freq = indexToFreq(i);
            float x = freqToX(freq, w);
            float y = dbToY(magnitudes[i], h);
            y = juce::jlimit(0.0f, h, y);

            if (first) {
                curvePath.startNewSubPath(x, y);
                fillPath.startNewSubPath(x, h); // bottom
                fillPath.lineTo(x, y);
                first = false;
            } else {
                curvePath.lineTo(x, y);
                fillPath.lineTo(x, y);
            }
        }

        // Close fill path
        fillPath.lineTo(w, h);
        fillPath.closeSubPath();

        // Gradient fill under curve
        juce::ColourGradient gradient(
            juce::Colour(0x6000b4d8), 0.0f, 0.0f,    // semi-transparent cyan at top
            juce::Colour(0x1000b4d8), 0.0f, h,         // nearly transparent at bottom
            false);
        g.setGradientFill(gradient);
        g.fillPath(fillPath);

        // Stroke the curve
        g.setColour(juce::Colour(0xff00b4d8)); // bright cyan
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));

        // Draw live spectrum (green) — uses its own dB range for visibility
        if (showSpectrum)
        {
            constexpr float specMinDb = -80.0f;
            constexpr float specMaxDb = 0.0f;

            juce::Path specPath;
            juce::Path specFill;
            bool specFirst = true;
            for (int i = 0; i < numPoints; ++i) {
                float freq = indexToFreq(i);
                float x = freqToX(freq, w);
                // Map spectrum dB to full component height
                float normY = (spectrumMagnitudes[i] - specMaxDb) / (specMinDb - specMaxDb);
                float y = juce::jlimit(0.0f, h, normY * h);

                if (specFirst) {
                    specPath.startNewSubPath(x, y);
                    specFill.startNewSubPath(x, h);
                    specFill.lineTo(x, y);
                    specFirst = false;
                } else {
                    specPath.lineTo(x, y);
                    specFill.lineTo(x, y);
                }
            }
            specFill.lineTo(w, h);
            specFill.closeSubPath();

            // Semi-transparent green fill
            juce::ColourGradient specGradient(
                juce::Colour(0x3066cc66), 0.0f, 0.0f,
                juce::Colour(0x0866cc66), 0.0f, h,
                false);
            g.setGradientFill(specGradient);
            g.fillPath(specFill);

            g.setColour(juce::Colour(0xaa66cc66)); // semi-transparent green
            g.strokePath(specPath, juce::PathStrokeType(1.0f));
        }

        // Cutoff frequency marker
        float cutoffX = freqToX(lastCutoff, w);
        g.setColour(juce::Colour(0x4000b4d8));
        g.drawVerticalLine((int)cutoffX, 0.0f, h);
    }

private:
    FilterModule& filterModule;
    std::vector<float> magnitudes;
    static constexpr int numPoints = 1024;
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -40.0f;
    static constexpr float maxDb = 50.0f;

    float lastCutoff = 0.0f;
    float lastResonance = 0.0f;
    float lastDrive = 0.0f;
    int lastFilterType = -1;

    // Spectrum analyzer
    bool showSpectrum = false;
    static constexpr int fftOrder = 10; // 1024-point FFT
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft{fftOrder};
    juce::dsp::WindowingFunction<float> window{fftSize, juce::dsp::WindowingFunction<float>::hann};
    std::vector<float> fftData;
    std::vector<float> spectrumMagnitudes;

    float indexToFreq(int i) const {
        float t = static_cast<float>(i) / static_cast<float>(numPoints - 1);
        return minFreq * std::pow(maxFreq / minFreq, t);
    }

    float freqToX(float freq, float width) const {
        float t = std::log(freq / minFreq) / std::log(maxFreq / minFreq);
        return t * width;
    }

    float dbToY(float db, float height) const {
        float normalized = (db - maxDb) / (minDb - maxDb); // 0 at top (maxDb), 1 at bottom (minDb)
        return normalized * height;
    }

    void recomputeMagnitudes() {
        float cutoff = lastCutoff;
        float resonance = lastResonance;
        int filterType = lastFilterType;

        if (cutoff < 20.0f) cutoff = 20.0f;

        float Q = 0.707f + resonance * 15.0f;

        for (int i = 0; i < numPoints; ++i) {
            float freq = indexToFreq(i);
            float mag = computeMagnitude(freq, cutoff, Q, filterType);
            float db = 20.0f * std::log10(std::max(mag, 0.0001f));
            magnitudes[i] = juce::jlimit(minDb, maxDb, db);
        }
    }

    float computeMagnitude(float freq, float cutoff, float Q, int filterType) const {
        float w = freq / cutoff;
        float w2 = w * w;
        float invQ = 1.0f / Q;
        float denom2 = (1.0f - w2) * (1.0f - w2) + w2 * invQ * invQ;
        float denom = std::sqrt(denom2);

        switch (filterType) {
        case 0: // LPF24
            { float lpf12 = 1.0f / denom; return lpf12 * lpf12; }
        case 1: // LPF12
            return 1.0f / denom;
        case 2: // HPF24
            { float hpf12 = w2 / denom; return hpf12 * hpf12; }
        case 3: // HPF12
            return w2 / denom;
        case 4: // BPF24
            { float bpf12 = (w * invQ) / denom; return bpf12 * bpf12; }
        case 5: // BPF12
            return (w * invQ) / denom;
        case 6: // Notch
            return std::abs(1.0f - w2) / denom;
        default:
            return 1.0f;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyResponseComponent)
};
