#include "MainComponent.h"
#include <gtest/gtest.h>
#include <juce_gui_basics/juce_gui_basics.h>

class MainComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // JUCE requires a MessageManager to be running for UI components
        juce::MessageManager::getInstance();
    }

    void TearDown() override {
        juce::MessageManager::deleteInstance();
        juce::DeletedAtShutdown::deleteAll();
    }
};

TEST_F(MainComponentTest, AIPanelIsVisibleByDefault) {
    MainComponent mainComp;
    EXPECT_TRUE(mainComp.isAiPanelConfiguredVisible());
}

TEST_F(MainComponentTest, ToggleAIPanelHidesAndShows) {
    MainComponent mainComp;

    // Initial state
    EXPECT_TRUE(mainComp.isAiPanelConfiguredVisible());

    // Click to hide
    mainComp.simulateToggleAiPanelClick();
    EXPECT_FALSE(mainComp.isAiPanelConfiguredVisible());

    // Click to show again
    mainComp.simulateToggleAiPanelClick();
    EXPECT_TRUE(mainComp.isAiPanelConfiguredVisible());
}

TEST_F(MainComponentTest, ModMatrixIsVisibleByDefault) {
    MainComponent mainComp;
    EXPECT_TRUE(mainComp.getGraphEditor().isModMatrixVisible());
}

TEST_F(MainComponentTest, ToggleModMatrixHidesAndShows) {
    MainComponent mainComp;

    // Initial state
    EXPECT_TRUE(mainComp.getGraphEditor().isModMatrixVisible());

    // Click to hide
    mainComp.simulateToggleModMatrixClick();
    EXPECT_FALSE(mainComp.getGraphEditor().isModMatrixVisible());

    // Click to show again
    mainComp.simulateToggleModMatrixClick();
    EXPECT_TRUE(mainComp.getGraphEditor().isModMatrixVisible());
}
