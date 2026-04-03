# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **modular synthesizer** application built with JUCE and C++20, featuring a node-based graph editor for sound design. The application allows users to create complex sounds by connecting audio modules in a visual patching environment.

## Architecture

The project follows a modular architecture with:

1. **Core Library (`GravisynthCore`)**: Contains all audio processing modules and core logic
2. **Application Layer (`Gravisynth`)**: GUI application built on JUCE that uses the core library
3. **UI Components**: Graph editor and module components for visual patching

### Key Components

- **ModuleBase**: Abstract base class for all audio modules with `ModuleType` enum, `ModulationTarget` metadata, and `VisualBuffer` support
- **AudioEngine**: Manages audio device I/O, the audio processor graph, and modulation matrix routing
- **GraphEditor**: Visual editor for connecting modules with zoom/pan and drag-to-connect
- **ModuleComponent**: Auto-UI generation from module parameters with type-safe layout switching via `ModuleType` enum
- **AttenuverterModule**: Intermediary module for modulation routing with bypass and CV amount control

### Audio Modules

- Oscillator: Waveform generator (Sine, Saw, Square, Triangle)
- Filter: Resonant low-pass filter with cutoff/resonance control
- VCA: Voltage Controlled Amplifier for dynamic control
- ADSR: Envelope generator for amplitude/filter modulation
- LFO: Low Frequency Oscillator for modulation
- Sequencer: Step sequencer with per-step pitch control
- MIDI Keyboard: Interactive on-screen keyboard for MIDI input
- FX Modules: Delay, Distortion, Reverb
- Preset System: Factory presets with categorized organization

## Build System

The project uses CMake with:
- JUCE framework (fetched automatically via FetchContent)
- GoogleTest for unit testing
- Code coverage support (enabled via `ENABLE_COVERAGE` option)

## Development Commands

### Build
```bash
cmake -S . -B build
cmake --build build
```

### Run Tests
```bash
cmake --build build --target GravisynthTests
./build/Tests/GravisynthTests
```

### Check Coverage
```bash
bash scripts/coverage.sh
```

## Testing Strategy

- Unit tests for individual modules using GoogleTest
- Tests cover audio processing behavior, parameter handling, and MIDI interaction
- Edge case tests (zero-length buffers, extreme parameters, sample rate changes)
- Integration tests (signal chains, modulation routing)
- Code coverage enforcement (threshold: 85%, CI pipeline enforces)
- ~165 tests across 28 suites
- CI runs on Ubuntu + macOS with linting, building, testing, and coverage

## Key Files to Understand

- `CMakeLists.txt`: Main build configuration (version 0.14.0)
- `Source/AudioEngine.h/cpp`: Audio processing engine, device management, and modulation matrix
- `Source/Modules/ModuleBase.h`: Base class with `ModuleType` enum, `ModulationTarget`, `ModulationCategory`
- `Source/Modules/OscillatorModule.h`: Oscillator with PolyBLEP/PolyBLAMP anti-aliasing, waveform crossfade, and CV feedback fix (channel 0 shared between CV input and audio output, saved before overwrite)
- `Source/Modules/VisualBuffer.h`: Thread-safe circular buffer using `std::atomic<float>`
- `Source/PresetManager.h/cpp`: Factory presets with categorized organization
- `Source/UI/ModuleComponent.cpp`: Auto-UI with type-safe `ModuleType` switching (no string comparisons)
- `Source/UI/GraphEditor.cpp`: Graph editor with attenuverter knob rendering and modulation routing
- `Tests/`: ~165 tests across 28 suites (unit, edge case, integration)