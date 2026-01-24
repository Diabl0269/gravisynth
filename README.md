# Gravisynth

Gravisynth is a modular synthesizer application built with JUCE and C++20.

## Overview

Gravisynth features a node-based audio processing graph where users can connect various modules to create complex sounds.

### Core Modules
- **Oscillator**: Basic waveform generator (Sine, Saw, Square, Triangle)
- **Filter**: Resonant low-pass filter
- **VCA**: Voltage Controlled Amplifier
- **ADSR**: Envelope generators
- **Sequencer**: Step sequencer for melody generation

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
- `Tests/`: Unit and integration tests (Coming Soon)

### Dependencies
- **JUCE**: Handled automatically via CMake FetchContent (v8.0.3)

## License
MIT
