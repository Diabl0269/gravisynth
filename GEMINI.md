# GEMINI - Developer Guide

This document is intended for AI agents and developers working on `Gravisynth`.

## Project Structure
- **GravisynthCore**: A static library containing all audio modules and logic. This is linked by both the main application and the test suite to ensure testability.
- **Gravisynth**: The standalone JUCE application (GUI + Audio).
- **Tests**: GoogleTest-based unit test suite.
- **docs**: Comprehensive documentation, including the AI Engine.

## Development Standards

### Code Style
- Follow the `.clang-format` configuration (LLVM based).
- Run `clang-format -i` on new files.

### Testing
- All new modules must have unit tests in `Tests/`.
- Key logic (DSP, State Management) must be tested.
- Run tests via CMake:
    ```bash
    cmake --build build --target GravisynthTests
    ./build/Tests/GravisynthTests
    ```

### Code Coverage
- **Threshold**: 90% Line Coverage is enforced. (Currently a target we are working towards.)
- Run the coverage script to verify standard compliance locally:
    ```bash
    bash scripts/coverage.sh
    ```
- This script builds the project with coverage flags, runs tests, and generates a report.

### CI/CD
- GitHub Actions workflow (`.github/workflows/ci.yml`) runs on every push to `main`.
- Checks: Linting, Build, Unit Tests, Coverage >90%.

### Documentation & Contribution
- **Consider Updates**: Before every commit/PR, consider if any relevant documentation (`README.md`, `GEMINI.md`, or the `docs/` folder) needs to be updated to reflect your changes.
- **AI Engine**: Detailed documentation on the AI Engine's architecture, components, and functionality is available in `docs/AI_Engine.md`.
- **AI Usage Guide**: A user-centric guide on how to effectively use the AI Sound Designer is available in `docs/AI_Usage_Guide.md`.
- **Module Development Guide**: Detailed guidance for creating new audio modules is available in `docs/Module_Development_Guide.md`.

## Module Architecture

All audio modules inherit from `ModuleBase`, which extends `juce::AudioProcessor`:

```cpp
class MyModule : public ModuleBase {
public:
    MyModule() : ModuleBase("MyModule", numInputs, numOutputs) { }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
};
```

### DSP Standards (PR #6+)
To ensure professional sound quality, all modules must implement:
- **Parameter Smoothing**: Use `juce::SmoothedValue` for all continuous parameters (Gain, Cutoff, etc.) to prevent clicking during automation.
- **Anti-Aliased Oscillators**: Use PolyBLEP or similar techniques for waveforms with Sharp edges (Square, Saw) to reduce aliasing artifacts.
- **Oversampling**: Use `juce::dsp::Oversampling` for non-linear modules (Distortion) to mitigate harmonic folding.

Key patterns:
- **Parameters**: Use `addParameter()` with `juce::AudioParameterFloat`, `AudioParameterChoice`, etc.
- **VisualBuffer**: Call `enableVisualBuffer(true)` for waveform visualization support.
- **State**: Override `getStateInformation()` / `setStateInformation()` for preset save/load.

## Registered Modules

### Core Modules
- **OscillatorModule**: Anti-aliased multi-waveform generator.
- **FilterModule**: Resonant low-pass filter.
- **ADSRModule**: Envelope generator.
- **LFOModule**: Modulation oscillator with sync/glide.
- **VCAModule**: Smoothed gain stage with CV control.
- **SequencerModule**: Mono step sequencer.

### Polyphonic Modules
- **PolyMidiModule**: 8-voice MIDI manager with LRU allocation.
- **PolySequencerModule**: Polyphonic step sequencer.

### FX Modules
- **DelayModule**: Interpolated feedback delay.
- **DistortionModule**: Oversampled soft-clipping distortion.
- **ReverbModule**: Stereo algorithmic reverb.

## AI Integration

Gravisynth features an AI-powered sound design assistant that can generate and modify patches using natural language. This "AI Engine" is primarily managed by the `AIIntegrationService`, which acts as the central orchestrator between the AI models and the core synthesizer audio graph. It handles sending user prompts to selected AI providers, interpreting AI-generated patch data (JSON), and applying these changes to the live audio processing graph. Furthermore, it manages chat history and can provide the current synth state as context to the AI for more intelligent patch generation.

### Components
- **`AIProvider`**: Abstract interface for AI backends (Ollama, etc.).
- **`OllamaProvider`**: Concrete implementation of `AIProvider` for local Ollama instances.
- **`AIIntegrationService`**: Orchestrates AI communication, maintains chat history, and bridges the AI with the audio engine.
- **`AIStateMapper`**: Handles the mapping between AI-friendly JSON and the internal synthesizer graph.

### Communication Pattern
The AI communicates using a simplified JSON schema describing nodes (id, type, params) and connections (src, dst, ports).

## Platform Notes
... (rest of the file)

### Coverage Tooling
| Platform | Command |
|----------|---------|
| macOS | `xcrun llvm-cov` / `xcrun llvm-profdata` |
| Linux (CI) | `llvm-cov` / `llvm-profdata` |

The `scripts/coverage.sh` uses `xcrun` for macOS. On Linux CI, we use the raw `llvm-*` commands.

### Include Paths
Tests use paths relative to `Source/`:
```cpp
#include "Modules/OscillatorModule.h"  // Correct
#include "OscillatorModule.h"          // Won't work
```

## Common Tasks
- **Adding a new module**:
    1.  Create `Source/Modules/NewModule.h`.
    2.  Add to `GravisynthCore` in `CMakeLists.txt`.
    3.  Create `Tests/NewModuleTests.cpp`.
    4.  Register test in `Tests/CMakeLists.txt`.
    5.  Run `scripts/coverage.sh` to verify.
