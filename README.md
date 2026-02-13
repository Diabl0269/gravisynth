# Gravisynth

A **modular synthesizer** application built with JUCE and C++20, featuring a node-based graph editor for sound design.

## Overview

Gravisynth provides a visual patching environment where audio modules can be freely connected to create complex sounds. Each module processes audio and/or control signals, enabling everything from simple subtractive synthesis to elaborate modulation chains.

### Module Graph
Modules connect via inputs and outputs:
```
[Sequencer] ──▶ [Oscillator] ──▶ [Filter] ──▶ [VCA] ──▶ [Output]
                    ▲               ▲           ▲
                 [LFO]          [ADSR]       [ADSR]
```

### Core Modules
- **Oscillator**: Anti-aliased waveform generator (Sine, Saw, Square, Triangle).
- **Filter**: Resonant low-pass filter with cutoff/resonance control.
- **VCA**: Voltage Controlled Amplifier with parameter smoothing.
- **ADSR**: Envelope generator for amplitude/filter modulation.
- **LFO**: Modulation oscillator with glide and S&H modes.
- **Sequencer**: Step sequencer for melodic patterns.

### FX Modules
- **Delay**: Interpolated delay with feedback and mix control.
- **Distortion**: 2x oversampled soft-clipping for harmonic warmth.
- **Reverb**: Lush algorithmic stereo reverb.

### Polyphony
- **Poly MIDI**: 8-voice voice management with LRU allocation.
- **Poly Sequencer**: Multi-voice pattern sequencing.

## Building
... (Build instructions remain same) ...

## Development

### Project Structure
- `Source/`: Main source code
    - `Modules/`: Audio processing modules (Core, FX, Poly)
    - `UI/`: Graph editor and visual components
- `Tests/`: GoogleTest suite
- `docs/`: **Technical Documentation (Architecture, Module Specs)**
- `GEMINI.md`: **Developer Guide & Contribution Standards**

... (Testing section) ...

## Roadmap

### Current Focus: UI/UX & Workflow
- [x] **FX Suite**: Delay, Distortion, Reverb.
- [x] **Anti-Aliasing**: High-quality oscillators.
- [ ] **Patch Saving/Loading**: Full state persistence for complex graphs.
- [ ] **Drag-and-Drop Patching**: Improved visual connection workflow.

### Advanced Features
- **Wavetable Synthesis**: Support for custom wavetables and morphine.
- **Advanced Modulation**: Matrix-style routing for complex sounds.

### Vision: AI-Powered Sound Design
... (AI section) ...
The flagship feature on our roadmap is an **AI Sound Designer** interface:
- Describe a sound in natural language (e.g., *"warm bass with slow attack"*)
- AI generates the complete patch: selects modules, configures parameters, and creates connections
- Iterate with conversational refinements

## License
MIT
