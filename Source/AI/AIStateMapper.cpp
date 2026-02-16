#include "AIStateMapper.h"
#include "../Modules/ADSRModule.h"
#include "../Modules/FX/DelayModule.h"
#include "../Modules/FX/DistortionModule.h"
#include "../Modules/FX/ReverbModule.h"
#include "../Modules/FilterModule.h"
#include "../Modules/LFOModule.h"
#include "../Modules/MidiKeyboardModule.h"
#include "../Modules/OscillatorModule.h"
#include "../Modules/SequencerModule.h"
#include "../Modules/VCAModule.h"
#include <functional> // For std::function
#include <map>
#include <unordered_map> // For the factory map

namespace gsynth {

typedef juce::AudioProcessorGraph::AudioGraphIOProcessor AudioGraphIOProcessor;

using ModuleFactoryFunc = std::function<std::unique_ptr<juce::AudioProcessor>()>;

// Factory map for module creation
static const std::unordered_map<juce::String, ModuleFactoryFunc> moduleFactory = {
    {"Audio Input", []() { return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioInputNode); }},
    {"Audio Output", []() { return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::audioOutputNode); }},
    {"Midi Input", []() { return std::make_unique<AudioGraphIOProcessor>(AudioGraphIOProcessor::midiInputNode); }},
    {"Oscillator", []() { return std::make_unique<OscillatorModule>(); }},
    {"Filter", []() { return std::make_unique<FilterModule>(); }},
    {"VCA", []() { return std::make_unique<VCAModule>(); }},
    {"ADSR", []() { return std::make_unique<ADSRModule>(); }}, // ADSR constructor for generic case
    {"Sequencer", []() { return std::make_unique<SequencerModule>(); }},
    {"LFO", []() { return std::make_unique<LFOModule>(); }},
    {"Distortion", []() { return std::make_unique<DistortionModule>(); }},
    {"Delay", []() { return std::make_unique<DelayModule>(); }},
    {"Reverb", []() { return std::make_unique<ReverbModule>(); }},
    {"MIDI Keyboard", []() { return std::make_unique<MidiKeyboardModule>(); }}};
