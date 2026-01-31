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

# Coverage Option
option(ENABLE_COVERAGE "Enable code coverage testing" OFF)

if(ENABLE_COVERAGE)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

# Create the core library
add_library(GravisynthCore STATIC
    Source/AudioEngine.cpp
    Source/AudioEngine.h
    # Modules
    Source/Modules/ModuleBase.h
    Source/Modules/OscillatorModule.h
    Source/Modules/ADSRModule.h
    Source/Modules/FilterModule.h
    Source/Modules/VCAModule.h
    Source/Modules/SequencerModule.h
    Source/Modules/LFOModule.h
    Source/Modules/VisualBuffer.h
    Source/Modules/FX/DelayModule.h
    Source/Modules/FX/DistortionModule.h
    Source/Modules/FX/ReverbModule.h
)

# Link Core against JUCE
target_link_libraries(GravisynthCore PUBLIC
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_dsp
)

target_include_directories(GravisynthCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

if(MSVC)
    target_compile_options(GravisynthCore PRIVATE /W4)
else()
    target_compile_options(GravisynthCore PRIVATE -Wall -Wextra)
endif()

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
    # UI (UI depends on Core, but technically could be in App for now)
    Source/UI/GraphEditor.cpp
    Source/UI/GraphEditor.h
    Source/UI/ModuleComponent.cpp
    Source/UI/ModuleComponent.h
)

# Link against JUCE modules
target_link_libraries(Gravisynth PRIVATE
    GravisynthCore # Link to our core library
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

add_subdirectory(Tests)

