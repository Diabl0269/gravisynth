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
    AntiClickTests.cpp
    MidiKeyboardModuleTests.cpp
    VCAModuleTests.cpp
    LFOModuleTests.cpp
    SequencerModuleTests.cpp
    AIStateMapperTests.cpp
    OllamaProviderTests.cpp
    MainComponentTests.cpp
    GraphEditorTests.cpp
    ModuleComponentTests.cpp
    AIChatComponentTests.cpp
    PolyMidiModuleTests.cpp
    PolySequencerModuleTests.cpp
    AIIntegrationServiceTests.cpp
    ModMatrixTests.cpp
    FXModuleTests.cpp
    VoiceMixerModuleTests.cpp
    EdgeCaseTests.cpp
    IntegrationTests.cpp
    UndoRedoTests.cpp
    PresetManagerTests.cpp
    ModuleBypassTests.cpp
    AudioRenderingTests.cpp
    WorkflowTests.cpp
    ../Source/MainComponent.cpp
    ../Source/UI/GraphEditor.cpp
    ../Source/UI/ModMatrixComponent.cpp
    ../Source/UI/AIChatComponent.cpp
    ../Source/UI/ModuleComponent.cpp
)

target_link_libraries(GravisynthTests PRIVATE
    GTest::gtest
    GravisynthCore
)

# Sources are now in GravisynthCore, so we don't need to add them here manually
# except for the test files themselves.

target_include_directories(GravisynthTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

target_compile_definitions(GravisynthTests PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)

# Link JUCE specifics if needed
target_link_libraries(GravisynthTests PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_audio_basics
    juce::juce_dsp
)

include(GoogleTest)
gtest_discover_tests(GravisynthTests)

# Separate test target for plugin tests to avoid symbol conflicts with main test binary
add_executable(GravisynthPluginTests
    PluginProcessorTests.cpp
    ../Source/Plugin/PluginProcessor.cpp
    ../Source/Plugin/PluginEditor.cpp
    ../Source/UI/GraphEditor.cpp
    ../Source/UI/ModMatrixComponent.cpp
    ../Source/UI/AIChatComponent.cpp
    ../Source/UI/ModuleComponent.cpp
)

target_link_libraries(GravisynthPluginTests PRIVATE
    GTest::gtest_main
    GravisynthCore
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_dsp
)

target_include_directories(GravisynthPluginTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Source
)

gtest_discover_tests(GravisynthPluginTests)