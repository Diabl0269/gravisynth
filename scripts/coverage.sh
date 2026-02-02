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
echo "Merging profile data..."

# Detect tools
if command -v xcrun &> /dev/null; then
    PROFDATA="xcrun llvm-profdata"
    COV="xcrun llvm-cov"
else
    PROFDATA="llvm-profdata"
    COV="llvm-cov"
fi

$PROFDATA merge -sparse default.profraw -o default.profdata

echo "Generating coverage report..."
# Capture the report output
REPORT=$($COV report \
    ./build/Tests/GravisynthTests \
    -instr-profile=default.profdata \
    -ignore-filename-regex="JuceLibraryCode|build/_deps|Tests")

echo "$REPORT"

# Parse total line coverage
# The line looks like: TOTAL ... 96.51% ...
# Column 10 corresponds to Line Coverage percentage in the standard report format.
TOTAL_COVERAGE=$(echo "$REPORT" | grep "TOTAL" | awk '{print $10}' | sed 's/%//')

echo "Total Line Coverage: $TOTAL_COVERAGE%"

# Threshold check (awk for float comparison to avoid installing bc)
PASS=$(awk -v cov="$TOTAL_COVERAGE" 'BEGIN {print (cov >= 38.28) ? 1 : 0}')

if [ "$PASS" -eq 1 ]; then
  echo "Coverage check passed."
  exit 0
else
  echo "Error: Coverage ($TOTAL_COVERAGE%) is below threshold (38.28%)"
  exit 1
fi
