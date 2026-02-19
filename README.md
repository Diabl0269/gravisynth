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
- **MIDI Keyboard**: Interactive on-screen keyboard for real-time performance.

### FX Modules
- **Delay**: Interpolated delay with feedback and mix control.
- **Distortion**: 2x oversampled soft-clipping for harmonic warmth.
- **Reverb**: Lush algorithmic stereo reverb.

### Polyphony
- **Poly MIDI**: 8-voice voice management with LRU allocation.
- **Poly Sequencer**: Multi-voice pattern sequencing.

### AI Integration [NEW]
- **AI Sound Designer**: Describe a sound in natural language and the AI generates the complete patch (modules, parameters, and connections).
- **One-Click Apply**: Instantly apply AI-generated patches to the graph editor.

## Building

Gravisynth uses CMake for its build system.

### Prerequisites
-   A C++20 compatible compiler (e.g., Clang, GCC, MSVC)
-   CMake (version 3.15 or newer)
-   JUCE (handled by CMake's FetchContent, no manual download needed)

### Steps

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/Diabl0269/gravisynth.git
    cd gravisynth
    ```

2.  **Configure CMake:**
    Create a build directory and configure CMake. You can choose `Debug` or `Release` build types.

    *   **Debug Build (for development):**
        ```bash
        cmake -Bbuild -S. -DCMAKE_BUILD_TYPE=Debug
        ```
    *   **Release Build (for deployment):**
        ```bash
        cmake -Bbuild -S. -DCMAKE_BUILD_TYPE=Release
        ```

3.  **Build the application:**
    ```bash
    cmake --build build --target Gravisynth
    ```
    This will compile the application. On macOS, the executable will be found at `build/Gravisynth_artefacts/<BuildType>/Gravisynth.app`. On other platforms, the path might vary (e.g., `build/<BuildType>/Gravisynth`).

4.  **Run the application:**
    *   **macOS:**
        ```bash
        open build/Gravisynth_artefacts/Release/Gravisynth.app
        # or
        open build/Gravisynth_artefacts/Debug/Gravisynth.app
        ```
    *   **Linux/Windows (example for Release):**
        ```bash
        ./build/Release/Gravisynth
        ```

## Development

### Project Structure
- `Source/`: Main source code
    - `Modules/`: Audio processing modules (Core, FX, Poly)
    - `UI/`: Graph editor and visual components
- `Tests/`: GoogleTest suite
- `docs/`: **Technical Documentation (Architecture, Module Specs)**
- `GEMINI.md`: **Developer Guide & Contribution Standards**

## Testing

Gravisynth uses GoogleTest for unit testing.

### Running Unit Tests

1.  **Build the test suite:**
    ```bash
    cmake --build build --target GravisynthTests
    ```

2.  **Execute the tests:**
    ```bash
    ./build/Tests/GravisynthTests
    ```

### Code Coverage

To generate a code coverage report (requires `llvm-cov` and `llvm-profdata`):

```bash
bash scripts/coverage.sh
```
This script will build the project with coverage flags, run the tests, and generate a detailed coverage report.

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
- [x] **Local AI Integration**: Support for Ollama and local models.
- [x] **Natural Language Patching**: Text-to-patch generation.
- [ ] **Conversational Refinement**: Iterate on patches via chat.
- [ ] **Parameter Learning**: Train models on user sound preferences.

## License
MIT
