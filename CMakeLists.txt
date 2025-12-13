cmake_minimum_required(VERSION 3.22)

project(Gravisynth VERSION 0.0.1)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.3 # Using a specific tag for stability
)
FetchContent_MakeAvailable(juce)

# Create the application target
juce_add_gui_app(Gravisynth
    PRODUCT_NAME "Gravisynth"
    VERSION "0.0.1"
)

juce_generate_juce_header(Gravisynth)

# Add source files
target_sources(Gravisynth PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/AudioEngine.cpp
    Source/AudioEngine.h
    # Modules
    Source/Modules/ModuleBase.h
    Source/Modules/OscillatorModule.h
    Source/Modules/ADSRModule.h
    Source/Modules/FilterModule.h
    Source/Modules/VCAModule.h
    Source/Modules/SequencerModule.h
    # UI
    Source/UI/GraphEditor.cpp
    Source/UI/GraphEditor.h
    Source/UI/ModuleComponent.cpp
    Source/UI/ModuleComponent.h
)

# Link against JUCE modules
target_link_libraries(Gravisynth PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
)

# Enable strict warnings to catch issues early
if(MSVC)
    target_compile_options(Gravisynth PRIVATE /W4)
else()
    target_compile_options(Gravisynth PRIVATE -Wall -Wextra)
endif()
