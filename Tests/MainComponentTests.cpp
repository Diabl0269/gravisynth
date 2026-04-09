#include "../Source/AI/AIProvider.h"
#include "MainComponent.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MockProvider : public gsynth::AIProvider {
public:
    juce::String getProviderName() const override { return "Mock"; }
    void fetchAvailableModels(std::function<void(const juce::StringArray&, bool)> callback) override {
        callback({"MockModel"}, true);
    }
    void sendPrompt(const std::vector<Message>&, CompletionCallback callback, const juce::var&) override {
        callback("Mock response.", true);
    }
    void setModel(const juce::String& name) override { model = name; }
    juce::String getCurrentModel() const override { return model; }

private:
    juce::String model = "MockModel";
};

class MainComponentTest : public ::testing::Test {};

TEST_F(MainComponentTest, AIPanelIsVisibleByDefault) {
    MainComponent mainComp(std::make_unique<MockProvider>());
    EXPECT_TRUE(mainComp.isAiPanelConfiguredVisible());
}

TEST_F(MainComponentTest, ToggleAIPanelHidesAndShows) {
    MainComponent mainComp(std::make_unique<MockProvider>());

    EXPECT_TRUE(mainComp.isAiPanelConfiguredVisible());

    mainComp.simulateToggleAiPanelClick();
    EXPECT_FALSE(mainComp.isAiPanelConfiguredVisible());

    mainComp.simulateToggleAiPanelClick();
    EXPECT_TRUE(mainComp.isAiPanelConfiguredVisible());
}

TEST_F(MainComponentTest, ModMatrixIsVisibleByDefault) {
    MainComponent mainComp(std::make_unique<MockProvider>());
    EXPECT_TRUE(mainComp.getGraphEditor().isModMatrixVisible());
}

TEST_F(MainComponentTest, ToggleModMatrixHidesAndShows) {
    MainComponent mainComp(std::make_unique<MockProvider>());

    EXPECT_TRUE(mainComp.getGraphEditor().isModMatrixVisible());

    mainComp.simulateToggleModMatrixClick();
    EXPECT_FALSE(mainComp.getGraphEditor().isModMatrixVisible());

    mainComp.simulateToggleModMatrixClick();
    EXPECT_TRUE(mainComp.getGraphEditor().isModMatrixVisible());
}
