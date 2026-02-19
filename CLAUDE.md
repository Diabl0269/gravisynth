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

- **ModuleBase**: Abstract base class for all audio modules, defining common interfaces
- **AudioEngine**: Manages audio device I/O and the audio processor graph
- **GraphEditor**: Visual editor for connecting modules
- **Module Components**: UI representations of each audio module

### Audio Modules

- Oscillator: Waveform generator (Sine, Saw, Square, Triangle)
- Filter: Resonant low-pass filter with cutoff/resonance control
- VCA: Voltage Controlled Amplifier for dynamic control
- ADSR: Envelope generator for amplitude/filter modulation
- LFO: Low Frequency Oscillator for modulation
- Sequencer: Step sequencer with per-step pitch control
- MIDI Keyboard: Interactive on-screen keyboard for MIDI input
- FX Modules: Delay, Distortion, Reverb

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
- Code coverage enforcement (currently at 38.28%, target is 90%)
- CI/CD pipeline enforces linting, building, testing, and coverage

## Key Files to Understand

- `CMakeLists.txt`: Main build configuration
- `Source/AudioEngine.h/cpp`: Audio processing engine and device management
- `Source/Modules/ModuleBase.h`: Base class for all audio modules
- `Source/Modules/OscillatorModule.h`: Example oscillator implementation
- `Source/MainComponent.h/cpp`: Main application UI
- `Tests/OscillatorTests.cpp`: Example unit test structure