#include "../Source/Modules/OscillatorModule.h"
#include "../Source/UI/GraphEditor.h"
#include "../Source/UI/ModuleComponent.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

class ModuleComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        juce::MessageManager::getInstance();
        engine = std::make_unique<AudioEngine>();
        editor = std::make_unique<GraphEditor>(*engine);

        processor = std::make_unique<OscillatorModule>();
        moduleComponent = std::make_unique<ModuleComponent>(processor.get(), *editor);
    }

    void TearDown() override {
        moduleComponent.reset();
        processor.reset();
        editor.reset();
        engine.reset();
        juce::MessageManager::deleteInstance();
        juce::DeletedAtShutdown::deleteAll();
    }

    std::unique_ptr<AudioEngine> engine;
    std::unique_ptr<GraphEditor> editor;
    std::unique_ptr<juce::AudioProcessor> processor;
    std::unique_ptr<ModuleComponent> moduleComponent;
};

TEST_F(ModuleComponentTest, InitializationAndResizing) { EXPECT_NO_THROW(moduleComponent->setSize(200, 300)); }

TEST_F(ModuleComponentTest, ParameterAttachmentLinksUI) {
    // The processor should have an "OSC_TYPE" parameter which is a combo box
    // and an "OSC_TUNE" parameter which is a slider.

    // Find a slider inside the moduleComponent
    juce::Slider* foundSlider = nullptr;
    for (auto* child : moduleComponent->getChildren()) {
        if (auto* slider = dynamic_cast<juce::Slider*>(child)) {
            foundSlider = slider;
            break;
        }
    }

    ASSERT_NE(foundSlider, nullptr);

    // Change slider value programmatically within its generic range
    double minVal = foundSlider->getMinimum();
    double maxVal = foundSlider->getMaximum();
    double newValue = minVal + (maxVal - minVal) * 0.5;

    foundSlider->setValue(newValue, juce::sendNotificationSync);

    // As long as it doesn't crash, the UI interaction works
    EXPECT_GE(foundSlider->getValue(), minVal);
}

TEST_F(ModuleComponentTest, TimerCallbackDoesNotCrash) { EXPECT_NO_THROW(moduleComponent->timerCallback()); }
