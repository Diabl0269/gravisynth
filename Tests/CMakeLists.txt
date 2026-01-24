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
    OscillatorTests.cpp
    FilterTests.cpp
    ADSRTests.cpp
    ModuleBaseTests.cpp
    VisualBufferTests.cpp
)

target_link_libraries(GravisynthTests PRIVATE
    GTest::gtest_main
    GravisynthCore
)

# Sources are now in GravisynthCore, so we don't need to add them here manually
# except for the test files themselves.

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
