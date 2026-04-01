#include "PresetManager.h"
#include "AI/AIStateMapper.h"

namespace gsynth {

PresetManager::PresetManager() {}
PresetManager::~PresetManager() {}

juce::StringArray PresetManager::getPresetNames() {
    return {"Default", "Simple Lead", "Ambient Pad", "Modulated Bass", "Step Sequence", "Polyphonic Keys (WIP)"};
}

bool PresetManager::loadPreset(int index, juce::AudioProcessorGraph& graph) {
    juce::String jsonStr = getPresetJSON(index);
    if (jsonStr.isEmpty())
        return false;

    auto json = juce::JSON::parse(jsonStr);
    return AIStateMapper::applyJSONToGraph(json, graph, true);
}

bool PresetManager::loadDefaultPreset(juce::AudioProcessorGraph& graph) { return loadPreset(0, graph); }

juce::String PresetManager::getPresetJSON(int index) {
    // UI Layout Strategy (1600x900 screen):
    // Standard Width: 280px. Sequencer Width: 510px. Keyboard Width: 500px.
    // Row 1: y=10. Row 2: y=450. Row 3: y=800.

    switch (index) {
    case 0: // Default - Matching original sound exactly
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1300, "y": 800}},
    {"id": 3, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Sine", "octave": 0}},
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 1000.0, "resonance": 1.0, "drive": 1.0}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "Amp Env", "position": {"x": 650, "y": 450}, "params": {"attack": 0.1, "decay": 0.1, "sustain": 0.8, "release": 0.5}},
    {"id": 7, "type": "Filter Env", "position": {"x": 950, "y": 450}, "params": {"attack": 0.1, "decay": 0.1, "sustain": 0.8, "release": 0.5}},
    {"id": 8, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false, "bpm": 120.0}},
    {"id": 10, "type": "Distortion", "position": {"x": 1250, "y": 10}, "params": {"drive": 0.5, "mix": 0.5}},
    {"id": 11, "type": "Delay", "position": {"x": 1250, "y": 250}, "params": {"time": 0.3, "feedback": 0.4, "mix": 0.3}},
    {"id": 12, "type": "Reverb", "position": {"x": 1250, "y": 550}, "params": {"roomSize": 0.5, "damping": 0.5, "wet": 0.33, "dry": 0.4, "width": 1.0}},
    {"id": 13, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 14, "type": "Attenuverter", "position": {"x": 650, "y": 350}, "params": {"amount": 1.0}},
    {"id": 15, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}}
  ],
  "connections": [
    {"src": 8, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 8, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 8, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 8, "srcPort": -1, "dst": 4, "dstPort": -1},
    {"src": 15, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 15, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 15, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 15, "srcPort": -1, "dst": 4, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 6, "srcPort": 0, "dst": 13, "dstPort": 0},
    {"src": 13, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 7, "srcPort": 0, "dst": 14, "dstPort": 0},
    {"src": 14, "srcPort": 0, "dst": 4, "dstPort": 1},
    {"src": 5, "srcPort": 0, "dst": 10, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 10, "dstPort": 1},
    {"src": 10, "srcPort": 0, "dst": 11, "dstPort": 0},
    {"src": 10, "srcPort": 1, "dst": 11, "dstPort": 1},
    {"src": 11, "srcPort": 0, "dst": 12, "dstPort": 0},
    {"src": 11, "srcPort": 1, "dst": 12, "dstPort": 1},
    {"src": 12, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 12, "srcPort": 1, "dst": 2, "dstPort": 1}
  ]
})";

    case 1: // Simple Lead
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1250, "y": 10}},
    {"id": 3, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Saw", "octave": 0}},
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 2000.0, "resonance": 2.0}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "ADSR", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.2, "sustain": 0.7, "release": 0.2}},
    {"id": 7, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 8, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 9, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false}}
  ],
  "connections": [
    {"src": 7, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 7, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 9, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 9, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 6, "srcPort": 0, "dst": 8, "dstPort": 0},
    {"src": 8, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 1}
  ]
})";

    case 2: // Ambient Pad
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1300, "y": 800}},
    {"id": 3, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Sine", "octave": -1}},
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 800.0, "resonance": 1.0}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "ADSR", "position": {"x": 650, "y": 450}, "params": {"attack": 1.5, "decay": 1.0, "sustain": 0.8, "release": 2.0}},
    {"id": 7, "type": "Delay", "position": {"x": 1250, "y": 10}, "params": {"time": 0.5, "feedback": 0.6, "mix": 0.4}},
    {"id": 8, "type": "Reverb", "position": {"x": 1250, "y": 400}, "params": {"roomSize": 0.9, "damping": 0.3, "wet": 0.5}},
    {"id": 9, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 10, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 11, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false}}
  ],
  "connections": [
    {"src": 9, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 9, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 6, "srcPort": 0, "dst": 10, "dstPort": 0},
    {"src": 10, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 5, "srcPort": 0, "dst": 7, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 7, "dstPort": 1},
    {"src": 7, "srcPort": 0, "dst": 8, "dstPort": 0},
    {"src": 7, "srcPort": 1, "dst": 8, "dstPort": 1},
    {"src": 8, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 8, "srcPort": 1, "dst": 2, "dstPort": 1}
  ]
})";

    case 3: // Modulated Bass
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1250, "y": 10}},
    {"id": 3, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Saw", "octave": -2}},
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 400.0, "resonance": 4.0}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "LFO", "position": {"x": 950, "y": 450}, "params": {"rateHz": 4.0, "shape": "Triangle", "level": 1.0, "bipolar": true}},
    {"id": 7, "type": "ADSR", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.1, "sustain": 0.2, "release": 0.1}},
    {"id": 8, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 9, "type": "Attenuverter", "position": {"x": 650, "y": 350}, "params": {"amount": 0.5}},
    {"id": 10, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 11, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false}}
  ],
  "connections": [
    {"src": 8, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 8, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 6, "srcPort": 0, "dst": 9, "dstPort": 0},
    {"src": 9, "srcPort": 0, "dst": 4, "dstPort": 1},
    {"src": 7, "srcPort": 0, "dst": 10, "dstPort": 0},
    {"src": 10, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 1}
  ]
})";

    case 4: // Step Sequence
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1250, "y": 10}},
    {"id": 3, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false, "bpm": 130.0}},
    {"id": 4, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Square", "octave": 0}},
    {"id": 5, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 3000.0, "resonance": 2.0}},
    {"id": 6, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 7, "type": "ADSR", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.2, "sustain": 0.0, "release": 0.1}},
    {"id": 8, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 9, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}}
  ],
  "connections": [
    {"src": 3, "srcPort": -1, "dst": 4, "dstPort": -1},
    {"src": 3, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 9, "srcPort": -1, "dst": 4, "dstPort": -1},
    {"src": 9, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 6, "dstPort": 0},
    {"src": 7, "srcPort": 0, "dst": 8, "dstPort": 0},
    {"src": 8, "srcPort": 0, "dst": 6, "dstPort": 1},
    {"src": 6, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 6, "srcPort": 0, "dst": 2, "dstPort": 1}
  ]
})";

    case 5: // Polyphonic Keys (Fixed Monophonic)
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1250, "y": 10}},
    {"id": 3, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Sine", "octave": 0}},
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 5000.0, "resonance": 1.0}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 7, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false}},
    {"id": 8, "type": "Amp Env", "position": {"x": 650, "y": 450}, "params": {"attack": 0.1, "decay": 0.1, "sustain": 0.8, "release": 0.5}},
    {"id": 9, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}}
  ],
  "connections": [
    {"src": 6, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 6, "srcPort": -1, "dst": 8, "dstPort": -1},
    {"src": 7, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 7, "srcPort": -1, "dst": 8, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 8, "srcPort": 0, "dst": 9, "dstPort": 0},
    {"src": 9, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 1}
  ]
})";

    default:
        return "";
    }
}

} // namespace gsynth
