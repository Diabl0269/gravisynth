# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **modular synthesizer** application built with JUCE and C++20, featuring a node-based graph editor for sound design. The application allows users to create complex sounds by connecting audio modules in a visual patching environment.

## Architecture

The project follows a modular architecture with:

1. **Core Library (`GravisynthCore`)**: Contains all audio processing modules and core logic
2. **Application Layer (`Gravisynth`)**: GUI application built on JUCE that uses the core library
3. **Plugin (`GravisynthPlugin`)**: VST3/AU plugin wrapping the core library for DAW integration
4. **UI Components**: Graph editor and module components for visual patching

### Key Components

- **ModuleBase**: Abstract base class for all audio modules with `ModuleType` enum, `ModulationTarget` metadata, and `VisualBuffer` support
- **IGraphManager**: Abstract interface decoupling UI from audio backend; implemented by both `AudioEngine` (standalone) and `PluginProcessor` (VST3/AU)
- **AudioEngine**: Manages audio device I/O, the audio processor graph, and modulation matrix routing (standalone app)
- **GraphEditor**: Visual editor for connecting modules with zoom/pan and drag-to-connect
- **ModuleComponent**: Auto-UI generation from module parameters with type-safe layout switching via `ModuleType` enum
- **AttenuverterModule**: Intermediary module for modulation routing with bypass and CV amount control
- **Port Labels**: Virtual `getInputPortLabel()`/`getOutputPortLabel()` on ModuleBase, overridden per-module for descriptive port names in the UI
- **GravisynthUndoManager**: Snapshot-based undo/redo system wrapping `juce::UndoManager`, captures full graph state on every change
- **AI Integration** (`Source/AI/`): AIIntegrationService orchestrates LLM-powered features via OllamaProvider; AIStateMapper translates graph state for AI context

### Audio Modules

- Oscillator: Waveform generator (Sine, Saw, Square, Triangle)
- Filter: Multi-mode filter with 7 types (LPF24, LPF12, HPF24, HPF12, BPF24, BPF12, Notch), cutoff/resonance control, and frequency response visualizer
- VCA: Voltage Controlled Amplifier for dynamic control
- ADSR: Envelope generator for amplitude/filter modulation
- LFO: Low Frequency Oscillator for modulation
- Sequencer: Step sequencer with per-step pitch control
- MIDI Keyboard: Interactive on-screen keyboard for MIDI input
- FX Modules: Delay, Distortion, Reverb, Chorus, Phaser, Compressor, Flanger, Limiter
- Preset System: Factory presets with categorized organization
- Poly MIDI: Polyphonic MIDI input handling
- Poly Sequencer: Polyphonic step sequencer

## Build System

The project uses CMake with:
- JUCE framework (fetched automatically via FetchContent)
- GoogleTest for unit testing
- Code coverage support (enabled via `ENABLE_COVERAGE` option)
- `JUCE_WEB_BROWSER=0` — WebBrowserComponent is unused; disabling removes WebKit/libsoup deps on Linux

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

### Build and Test (Release)
A pre-push git hook automatically runs Release build + tests before every push. Install it with:
```bash
bash scripts/install-hooks.sh
```
The first push configures the `build-release` directory; subsequent pushes do fast incremental rebuilds. This catches UB/segfaults that only manifest with optimizations enabled (Debug mode hides use-after-free by zero-initializing memory).

To run manually:
```bash
bash scripts/pre-push-release-test.sh
```

### Check Coverage
```bash
bash scripts/coverage.sh
```

### Run E2E Tests
```bash
# Audio rendering tests only
./build/Tests/GravisynthTests --gtest_filter="AudioRendering*"

# Workflow tests only  
./build/Tests/GravisynthTests --gtest_filter="Workflow*"

# Snapshot/reference tests only
./build/Tests/GravisynthTests --gtest_filter="*Snapshot*"

# Regenerate reference files after intentional DSP changes
bash scripts/update-reference.sh

# Convert reference files to WAV for listening
bash scripts/play-reference.sh
```

### Git Hooks
```bash
bash scripts/install-hooks.sh    # Install pre-push hook (runs Release build+test before push)
```

## CI Pipeline

CI runs via `.github/workflows/ci.yml` on PRs only (4 parallel jobs):
- **Lint** (Ubuntu, ~30s) — clang-format check
- **Build, Test, and Coverage** (Ubuntu Debug) — coverage threshold 80%
- **Build and Test** (macOS) — Release build, catches UB/segfaults + cross-platform
- **Build and Test** (Windows) — Release build, catches UB/segfaults + cross-platform

Post-merge, `.github/workflows/build-artifacts.yml` runs on push to main (4 jobs): build+package on Ubuntu/macOS/Windows (no tests — CI already ran them), then tag+release.

**Optimizations:**
- **ccache**: Compiler cache avoids recompiling unchanged files. `CMAKE_C/CXX_COMPILER_LAUNCHER=ccache`, cached at `~/.ccache` (Linux) / `~/Library/Caches/ccache` (macOS), 500M max, keyed by commit SHA with prefix restore.
- **FetchContent caching**: `build/_deps` (JUCE 8.0.3 + GoogleTest 1.14.0) cached via `actions/cache`, keyed on `CMakeLists.txt` + `Tests/CMakeLists.txt` hashes.
- **JUCE_WEB_BROWSER=0**: Disabled unused WebBrowserComponent, removed WebKit/libsoup deps from Linux builds.
- **coverage.sh --report-only**: In CI, skips redundant configure/build/test and only does profdata merge + report.
- **Separate lint job**: Instant formatting feedback (~30s) without waiting for full build.
- **apt package caching**: `awalsh128/cache-apt-pkgs-action` caches Ubuntu packages across runs.
- **Path filtering**: PRs only trigger CI when `Source/`, `Tests/`, `CMakeLists.txt`, `scripts/`, or the workflow file change. Push to main always runs.

**What didn't work:** Unity builds (`CMAKE_UNITY_BUILD`) are incompatible with JUCE — Obj-C++ `.mm` files can't be merged into C++ unity translation units.

**What didn't work:** Precompiled headers (PCH) — JUCE compiles its own module `.cpp` files as part of the target and they have guards against being pre-included. On macOS, `.mm` files also need Obj-C++ mode which conflicts with C++ PCH.

## Testing Strategy

- Unit tests for individual modules using GoogleTest
- Tests cover audio processing behavior, parameter handling, and MIDI interaction
- Edge case tests (zero-length buffers, extreme parameters, sample rate changes)
- Integration tests (signal chains, modulation routing)
- Code coverage enforcement (threshold: 80%, CI pipeline enforces via `scripts/coverage.sh`)
- ~250 tests across 37 suites (unit, edge case, integration, undo/redo, filter modes, port labels)
- CI runs on Ubuntu + macOS with linting, building, testing, and coverage
- **Audio rendering tests** (`Tests/AudioRenderingTests.cpp`): End-to-end signal chain tests that process audio through modules and verify output. Tests cover oscillator output, frequency accuracy, signal chains (Osc→Filter→VCA), sequencer-driven patches, all 8 FX modules, preset rendering, ADSR envelope shape, LFO modulation, and modulation parameter changes
- **Reference snapshot tests**: Binary `.raw` files in `Tests/reference/` store expected audio output. Tests compare rendered audio sample-by-sample (tolerance: 1e-5) to catch subtle DSP regressions. Auto-generated on first run; regenerate with `bash scripts/update-reference.sh`
- **Workflow tests** (`Tests/WorkflowTests.cpp`): Multi-step UI interaction tests using JUCE component APIs. Tests cover module drop/delete, undo/redo, connection management, mod routing, preset loading, and module replacement
- **Test helpers** (`Tests/TestAudioHelpers.h`): Shared utilities for rendering modules, computing RMS, estimating frequency, silence detection, and reference file I/O
- All tests run headlessly — no audio device, no display server needed

## Key Files to Understand

- `CMakeLists.txt`: Main build configuration (version 0.13.2)
- `Source/AudioEngine.h/cpp`: Audio processing engine, device management, and modulation matrix
- `Source/GravisynthUndoManager.h/cpp`: Snapshot-based undo/redo with `SnapshotAction`, safe detach/reattach lifecycle
- `Source/Modules/ModuleBase.h`: Base class with `ModuleType` enum, `ModulationTarget`, `ModulationCategory`
- `Source/Modules/OscillatorModule.h`: Oscillator with PolyBLEP/PolyBLAMP anti-aliasing, waveform crossfade, and CV feedback fix (channel 0 shared between CV input and audio output, saved before overwrite)
- `Source/Modules/FilterModule.h`: Multi-mode filter (LadderFilter for LPF/HPF/BPF + SVF for notch), atomic modulated params for visualizer, type parameter
- `Source/Modules/VisualBuffer.h`: Thread-safe circular buffer using `std::atomic<float>`
- `Source/PresetManager.h/cpp`: Factory presets with categorized organization
- `Source/UI/ModuleComponent.cpp`: Auto-UI with type-safe `ModuleType` switching, parameter listener for undo, safe detach lifecycle, FrequencyResponseComponent integration and spectrum toggle
- `Source/UI/FrequencyResponseComponent.h`: Serum-style frequency response curve with FFT spectrum overlay
- `Source/UI/GraphEditor.cpp`: Graph editor with attenuverter knob rendering, modulation routing, and undo integration
- `Source/UI/ModuleLibraryComponent.h`: Categorized sidebar with section headers for module drag-and-drop
- `Source/UI/ModMatrixComponent.cpp`: Modulation matrix with undo tracking for routing and parameter changes
- `Source/IGraphManager.h`: Abstract interface for graph management, decouples UI from AudioEngine/PluginProcessor
- `Source/Plugin/PluginProcessor.h/cpp`: VST3/AU plugin processor wrapping `AudioProcessorGraph`, implements `IGraphManager`
- `Source/Plugin/PluginEditor.h/cpp`: Plugin editor embedding GraphEditor, ModuleLibrary, and AI chat UI
- `Source/Modules/FX/ChorusModule.h`: Chorus effect using `juce::dsp::Chorus`, CV modulation on Rate/Depth
- `Source/Modules/FX/PhaserModule.h`: Phaser effect using `juce::dsp::Phaser`, CV modulation on Rate/Depth
- `Source/Modules/FX/CompressorModule.h`: Compressor with manual makeup gain
- `Source/Modules/FX/FlangerModule.h`: Flanger via `juce::dsp::Chorus` with low-delay tuning
- `Source/Modules/FX/LimiterModule.h`: Brickwall limiter with input gain drive
- `Source/UI/AIChatComponent.cpp/.h`: Chat interface for AI-assisted patching
- `Source/UI/ScopeComponent.h`: Oscilloscope/waveform display component
- `Tests/`: ~250 tests across 37 suites (unit, edge case, integration, undo/redo, filter modes, port labels)
- `Tests/AudioRenderingTests.cpp`: E2E audio rendering tests with signal chain verification and snapshot comparison
- `Tests/WorkflowTests.cpp`: Multi-step UI workflow tests (undo/redo, module lifecycle, preset loading)
- `Tests/TestAudioHelpers.h`: Shared test utilities (rendering, RMS, frequency estimation, reference I/O)
- `Tests/reference/`: Binary reference audio files for snapshot comparison tests
- `scripts/update-reference.sh`: Regenerates audio reference files (run when DSP changes are intentional)
