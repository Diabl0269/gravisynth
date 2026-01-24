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

## Common Tasks
- **Adding a new module**:
    1.  Create `Source/Modules/NewModule.h`.
    2.  Add to `GravisynthCore` in `CMakeLists.txt`.
    3.  Create `Tests/NewModuleTests.cpp`.
    4.  Register test in `Tests/CMakeLists.txt`.
    5.  Run `scripts/coverage.sh` to verify.
