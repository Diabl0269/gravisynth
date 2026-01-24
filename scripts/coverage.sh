#!/bin/bash
set -e

# Configure with coverage enabled
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON

# Build the tests
cmake --build build --target GravisynthTests

# Run the tests
export LLVM_PROFILE_FILE="default.profraw"
./build/Tests/GravisynthTests

# Generate coverage report
# Note: Adjust the path if necessary depending on where the object files are located
echo "Merging profile data..."
xcrun llvm-profdata merge -sparse default.profraw -o default.profdata

echo "Generating coverage report..."
# Capture the report output
REPORT=$(xcrun llvm-cov report \
    ./build/Tests/GravisynthTests \
    -instr-profile=default.profdata \
    -ignore-filename-regex="JuceLibraryCode|build/_deps|Tests")

echo "$REPORT"

# Parse total line coverage
# The line looks like: TOTAL ... 96.51% ...
# Column 10 corresponds to Line Coverage percentage in the standard report format.
TOTAL_COVERAGE=$(echo "$REPORT" | grep "TOTAL" | awk '{print $10}' | sed 's/%//')

echo "Total Line Coverage: $TOTAL_COVERAGE%"

# Threshold check (bc for float comparison)
if (( $(echo "$TOTAL_COVERAGE < 90.0" | bc -l) )); then
  echo "Error: Coverage ($TOTAL_COVERAGE%) is below threshold (90%)"
  exit 1
else
  echo "Coverage check passed."
  exit 0
fi
