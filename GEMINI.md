# GEMINI - Developer Guide

This document is intended for AI agents and developers working on `Gravisynth`.

## Project Structure
- **GravisynthCore**: A static library containing all audio modules and logic. This is linked by both the main application and the test suite to ensure testability.
- **Gravisynth**: The standalone JUCE application (GUI + Audio).
- **Tests**: GoogleTest-based unit test suite.

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
- **Threshold**: 90% Line Coverage is enforced.
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

Key patterns:
- **Parameters**: Use `addParameter()` with `juce::AudioParameterFloat`, `AudioParameterChoice`, etc.
- **VisualBuffer**: Call `enableVisualBuffer(true)` for waveform visualization support.
- **State**: Override `getStateInformation()` / `setStateInformation()` for preset save/load.

## Platform Notes

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
