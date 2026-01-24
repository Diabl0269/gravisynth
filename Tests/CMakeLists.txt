enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(GravisynthTests
    TestMain.cpp
)

target_link_libraries(GravisynthTests PRIVATE
    GTest::gtest_main
    # Gravisynth - Cannot link executable
)

# For now, if Gravisynth is an executable, we can't link to it easily without object libraries.
# Alternative: Add source files again
# Note: AudioEngine depends on all modules, so we'd need to add them all or create a lib.
# For this first pass, let's just make sure GTest runs.
# target_sources(GravisynthTests PRIVATE
#     ../Source/AudioEngine.cpp
# )

target_include_directories(GravisynthTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

# Link JUCE specifics if needed
target_link_libraries(GravisynthTests PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_dsp
)

include(GoogleTest)
gtest_discover_tests(GravisynthTests)
