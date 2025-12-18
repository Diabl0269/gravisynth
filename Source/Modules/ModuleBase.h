#pragma once

#include <JuceHeader.h>

class ModuleBase : public juce::AudioProcessor {
public:
  ModuleBase(const juce::String &name, int numInputs, int numOutputs)
      : AudioProcessor(BusesProperties()
                           .withInput("Input", juce::AudioChannelSet::stereo(),
                                      numInputs > 0)
                           .withOutput("Output",
                                       juce::AudioChannelSet::stereo(),
                                       numOutputs > 0)),
        moduleName(name) {}

  ~ModuleBase() override = default;

  const juce::String getName() const override { return moduleName; }

  void prepareToPlay(double sampleRate, int samplesPerBlock) override = 0;
  void releaseResources() override {}
  void processBlock(juce::AudioBuffer<float> &buffer,
                    juce::MidiBuffer &midiMessages) override = 0;

  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor *createEditor() override {
    return nullptr;
  } // To be implemented later

  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return true; }
  double getTailLengthSeconds() const override { return 0.0; }

  // Boilerplate
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override { juce::ignoreUnused(index); }
  const juce::String getProgramName(int index) override {
    juce::ignoreUnused(index);
    return {};
  }
  void changeProgramName(int index, const juce::String &newName) override {
    juce::ignoreUnused(index, newName);
  }
  void getStateInformation(juce::MemoryBlock &destData) override {
    juce::ValueTree state("ModuleState");
    for (auto *param : getParameters()) {
      if (auto *p =
              dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
        state.setProperty(p->paramID, p->getValue(), nullptr);
      }
    }
    copyXmlToBinary(*state.createXml(), destData);
  }

  void setStateInformation(const void *data, int sizeInBytes) override {
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState != nullptr) {
      if (xmlState->hasTagName("ModuleState")) {
        juce::ValueTree state = juce::ValueTree::fromXml(*xmlState);
        for (auto *param : getParameters()) {
          if (auto *p =
                  dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
            if (state.hasProperty(p->paramID)) {
              p->setValue((float)state.getProperty(p->paramID));
            }
          }
        }
      }
    }
  }

private:
  juce::String moduleName;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModuleBase)
};
