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
- **Oscillator**: Waveform generator (Sine, Saw, Square, Triangle)
- **Filter**: Resonant low-pass filter with cutoff/resonance control
- **VCA**: Voltage Controlled Amplifier for dynamic control
- **ADSR**: Envelope generator for amplitude/filter modulation
- **LFO**: Low Frequency Oscillator for modulation
- **Sequencer**: Step sequencer with per-step pitch control

## Building

### Prerequisites
- CMake 3.22 or higher
- C++20 compatible compiler (Clang, GCC, MSVC)
- Git

### Build Instructions

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/gravisynth.git
   cd gravisynth
   ```

2. Configure with CMake:
   ```bash
   cmake -S . -B build
   ```

3. Build:
   ```bash
   cmake --build build
   ```

## Development

### Project Structure
- `Source/`: Main source code
    - `Modules/`: Audio processing modules
    - `UI/`: User interface components
- `Tests/`: Unit and integration tests
- `GEMINI.md`: **Developer Guide & Contribution Standards**

### Testing & CI
This project uses GoogleTest for unit testing and enforced 90% code coverage.

**Run Tests:**
```bash
cmake --build build --target GravisynthTests
./build/Tests/GravisynthTests
```

**Check Coverage:**
```bash
bash scripts/coverage.sh
```

**CI/CD:**
GitHub Actions enforces linting and coverage on every push. See `GEMINI.md` for details.

### Dependencies
- **JUCE**: Handled automatically via CMake FetchContent (v8.0.3)

## Roadmap

### Planned Features
- **Polyphonic Support**: Multi-voice synthesis with voice allocation and unison modes
- **Enhanced UI/UX**: Improved visual design, keyboard shortcuts, and workflow optimizations
- **Sound Quality**: Anti-aliased oscillators, oversampling, and additional filter types

### Vision: AI-Powered Sound Design
The flagship feature on our roadmap is an **AI Sound Designer** interface:
- Describe a sound in natural language (e.g., *"warm bass with slow attack"*)
- AI generates the complete patch: selects modules, configures parameters, and creates connections
- Iterate with conversational refinements

## License
MIT
