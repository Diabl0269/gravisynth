#include "PresetManager.h"
#include "AI/AIStateMapper.h"

namespace gsynth {

juce::StringArray PresetManager::getPresetNames() {
    return {"Default", "Simple Lead", "Ambient Pad", "Modulated Bass", "Step Sequence", "Electric Keys", "Poly Pad"};
}

juce::Array<PresetInfo> PresetManager::getPresetList() {
    juce::Array<PresetInfo> presets;
    presets.add({"Default", "Init"});
    presets.add({"Simple Lead", "Lead"});
    presets.add({"Ambient Pad", "Pad"});
    presets.add({"Modulated Bass", "Bass"});
    presets.add({"Step Sequence", "Sequence"});
    presets.add({"Electric Keys", "Keys"});
    presets.add({"Poly Pad", "Pad"});
    return presets;
}

juce::StringArray PresetManager::getCategories() { return {"Init", "Lead", "Pad", "Bass", "Sequence", "Keys"}; }

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
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 1000.0, "resonance": 0.1, "drive": 1.0}},
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
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 800.0, "resonance": 0.3}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "Amp Env", "position": {"x": 350, "y": 450}, "params": {"attack": 0.01, "decay": 0.2, "sustain": 0.7, "release": 0.3}},
    {"id": 7, "type": "Filter Env", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.3, "sustain": 0.3, "release": 0.2}},
    {"id": 8, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 9, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 10, "type": "Attenuverter", "position": {"x": 650, "y": 350}, "params": {"amount": 0.7}},
    {"id": 11, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false}}
  ],
  "connections": [
    {"src": 8, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 8, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 8, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 6, "dstPort": -1},
    {"src": 11, "srcPort": -1, "dst": 7, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 6, "srcPort": 0, "dst": 9, "dstPort": 0},
    {"src": 9, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 7, "srcPort": 0, "dst": 10, "dstPort": 0},
    {"src": 10, "srcPort": 0, "dst": 4, "dstPort": 1},
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
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 800.0, "resonance": 0.1}},
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
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 600.0, "resonance": 0.15}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.9}},
    {"id": 6, "type": "LFO", "position": {"x": 950, "y": 450}, "params": {"rateHz": 1.5, "shape": "Triangle", "level": 1.0, "bipolar": true}},
    {"id": 7, "type": "ADSR", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.3, "sustain": 0.8, "release": 0.3}},
    {"id": 8, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 9, "type": "Attenuverter", "position": {"x": 650, "y": 350}, "params": {"amount": 0.4}},
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
    {"id": 3, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": true, "bpm": 128.0, "Pitch 1": 48, "Pitch 2": 51, "Pitch 3": 55, "Pitch 4": 58, "Pitch 5": 60, "Pitch 6": 55, "Pitch 7": 51, "Pitch 8": 50, "Gate 1": 0.5, "Gate 2": 0.5, "Gate 3": 0.5, "Gate 4": 0.5, "Gate 5": 0.8, "Gate 6": 0.3, "Gate 7": 0.3, "Gate 8": 0.5}},
    {"id": 4, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Saw", "octave": -1}},
    {"id": 5, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 1500.0, "resonance": 0.2}},
    {"id": 6, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 7, "type": "ADSR", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.15, "sustain": 0.0, "release": 0.05}},
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

    case 5: // Electric Keys
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1250, "y": 10}},
    {"id": 3, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Triangle", "octave": 0}},
    {"id": 4, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 2000.0, "resonance": 0.1}},
    {"id": 5, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8}},
    {"id": 6, "type": "MIDI Keyboard", "position": {"x": 10, "y": 850}},
    {"id": 7, "type": "Sequencer", "position": {"x": 10, "y": 450}, "params": {"run": false}},
    {"id": 8, "type": "Amp Env", "position": {"x": 350, "y": 450}, "params": {"attack": 0.01, "decay": 0.4, "sustain": 0.4, "release": 0.5}},
    {"id": 9, "type": "Filter Env", "position": {"x": 650, "y": 450}, "params": {"attack": 0.01, "decay": 0.2, "sustain": 0.1, "release": 0.3}},
    {"id": 10, "type": "Attenuverter", "position": {"x": 950, "y": 350}, "params": {"amount": 1.0}},
    {"id": 11, "type": "Attenuverter", "position": {"x": 650, "y": 350}, "params": {"amount": 0.6}}
  ],
  "connections": [
    {"src": 6, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 6, "srcPort": -1, "dst": 8, "dstPort": -1},
    {"src": 6, "srcPort": -1, "dst": 9, "dstPort": -1},
    {"src": 7, "srcPort": -1, "dst": 3, "dstPort": -1},
    {"src": 7, "srcPort": -1, "dst": 8, "dstPort": -1},
    {"src": 7, "srcPort": -1, "dst": 9, "dstPort": -1},
    {"src": 3, "srcPort": 0, "dst": 4, "dstPort": 0},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 8, "srcPort": 0, "dst": 10, "dstPort": 0},
    {"src": 10, "srcPort": 0, "dst": 5, "dstPort": 1},
    {"src": 9, "srcPort": 0, "dst": 11, "dstPort": 0},
    {"src": 11, "srcPort": 0, "dst": 4, "dstPort": 1},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 5, "srcPort": 0, "dst": 2, "dstPort": 1}
  ]
})";

    case 6: // Poly Pad - 8-voice polyphonic pad
        // VCA sums all 8 voices to stereo (ch0/ch1) internally; no VoiceMixer needed.
        return R"({
  "nodes": [
    {"id": 1, "type": "Audio Input", "position": {"x": 10, "y": 10}},
    {"id": 2, "type": "Audio Output", "position": {"x": 1400, "y": 450}},
    {"id": 3, "type": "MIDI Keyboard", "position": {"x": 10, "y": 700}},
    {"id": 4, "type": "Poly MIDI", "position": {"x": 10, "y": 300}},
    {"id": 5, "type": "Oscillator", "position": {"x": 350, "y": 10}, "params": {"waveform": "Saw", "poly": true, "unison": 2, "detune": 10.0, "level": 0.7}},
    {"id": 6, "type": "Filter", "position": {"x": 650, "y": 10}, "params": {"cutoff": 1200.0, "resonance": 0.15, "poly": true}},
    {"id": 7, "type": "VCA", "position": {"x": 950, "y": 10}, "params": {"gain": 0.8, "poly": true}},
    {"id": 8, "type": "Amp Env", "position": {"x": 350, "y": 450}, "params": {"attack": 0.3, "decay": 0.4, "sustain": 0.7, "release": 1.5, "poly": true}},
    {"id": 10, "type": "Reverb", "position": {"x": 1200, "y": 10}, "params": {"roomSize": 0.8, "damping": 0.3, "wet": 0.5, "dry": 0.5, "width": 1.0}}
  ],
  "connections": [
    {"src": 3, "srcPort": -1, "dst": 4, "dstPort": -1},
    {"src": 3, "srcPort": -1, "dst": 5, "dstPort": -1},
    {"src": 3, "srcPort": -1, "dst": 8, "dstPort": -1},
    {"src": 4, "srcPort": 0, "dst": 5, "dstPort": 0},
    {"src": 4, "srcPort": 1, "dst": 5, "dstPort": 1},
    {"src": 4, "srcPort": 2, "dst": 5, "dstPort": 2},
    {"src": 4, "srcPort": 3, "dst": 5, "dstPort": 3},
    {"src": 4, "srcPort": 4, "dst": 5, "dstPort": 4},
    {"src": 4, "srcPort": 5, "dst": 5, "dstPort": 5},
    {"src": 4, "srcPort": 6, "dst": 5, "dstPort": 6},
    {"src": 4, "srcPort": 7, "dst": 5, "dstPort": 7},
    {"src": 4, "srcPort": 8, "dst": 8, "dstPort": 0},
    {"src": 4, "srcPort": 9, "dst": 8, "dstPort": 1},
    {"src": 4, "srcPort": 10, "dst": 8, "dstPort": 2},
    {"src": 4, "srcPort": 11, "dst": 8, "dstPort": 3},
    {"src": 4, "srcPort": 12, "dst": 8, "dstPort": 4},
    {"src": 4, "srcPort": 13, "dst": 8, "dstPort": 5},
    {"src": 4, "srcPort": 14, "dst": 8, "dstPort": 6},
    {"src": 4, "srcPort": 15, "dst": 8, "dstPort": 7},
    {"src": 5, "srcPort": 0, "dst": 6, "dstPort": 0},
    {"src": 5, "srcPort": 1, "dst": 6, "dstPort": 1},
    {"src": 5, "srcPort": 2, "dst": 6, "dstPort": 2},
    {"src": 5, "srcPort": 3, "dst": 6, "dstPort": 3},
    {"src": 5, "srcPort": 4, "dst": 6, "dstPort": 4},
    {"src": 5, "srcPort": 5, "dst": 6, "dstPort": 5},
    {"src": 5, "srcPort": 6, "dst": 6, "dstPort": 6},
    {"src": 5, "srcPort": 7, "dst": 6, "dstPort": 7},
    {"src": 6, "srcPort": 0, "dst": 7, "dstPort": 0},
    {"src": 6, "srcPort": 1, "dst": 7, "dstPort": 1},
    {"src": 6, "srcPort": 2, "dst": 7, "dstPort": 2},
    {"src": 6, "srcPort": 3, "dst": 7, "dstPort": 3},
    {"src": 6, "srcPort": 4, "dst": 7, "dstPort": 4},
    {"src": 6, "srcPort": 5, "dst": 7, "dstPort": 5},
    {"src": 6, "srcPort": 6, "dst": 7, "dstPort": 6},
    {"src": 6, "srcPort": 7, "dst": 7, "dstPort": 7},
    {"src": 8, "srcPort": 0, "dst": 7, "dstPort": 8},
    {"src": 8, "srcPort": 1, "dst": 7, "dstPort": 9},
    {"src": 8, "srcPort": 2, "dst": 7, "dstPort": 10},
    {"src": 8, "srcPort": 3, "dst": 7, "dstPort": 11},
    {"src": 8, "srcPort": 4, "dst": 7, "dstPort": 12},
    {"src": 8, "srcPort": 5, "dst": 7, "dstPort": 13},
    {"src": 8, "srcPort": 6, "dst": 7, "dstPort": 14},
    {"src": 8, "srcPort": 7, "dst": 7, "dstPort": 15},
    {"src": 7, "srcPort": 0, "dst": 10, "dstPort": 0},
    {"src": 7, "srcPort": 1, "dst": 10, "dstPort": 1},
    {"src": 10, "srcPort": 0, "dst": 2, "dstPort": 0},
    {"src": 10, "srcPort": 1, "dst": 2, "dstPort": 1}
  ]
})";

    default:
        return "";
    }
}

} // namespace gsynth
