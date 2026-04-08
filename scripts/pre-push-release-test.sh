#!/bin/bash
set -e

BUILD_DIR="build-release"

echo "Pre-push: Running Release build + tests..."

# Configure on first run
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "Pre-push: Configuring Release build (first run)..."
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

# Incremental build
cmake --build "$BUILD_DIR" --target GravisynthTests -- -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

# Run tests
"$BUILD_DIR/Tests/GravisynthTests"

echo "Pre-push: All tests passed in Release mode."
