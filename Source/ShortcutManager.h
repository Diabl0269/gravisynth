#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace GravisynthCommands {
enum CommandIDs { openSettings = 0x100, savePreset, openPreset, undo, redo };

inline juce::CommandID getCommandForAction(const juce::String& actionId) {
    if (actionId == "openSettings")
        return openSettings;
    if (actionId == "savePreset")
        return savePreset;
    if (actionId == "openPreset")
        return openPreset;
    if (actionId == "undo")
        return undo;
    if (actionId == "redo")
        return redo;
    return 0;
}
} // namespace GravisynthCommands

class ShortcutManager {
public:
    ShortcutManager() { resetToDefaults(); }

    void loadFromProperties(juce::ApplicationProperties& props) {
        appProperties = &props;
        auto* settings = props.getUserSettings();
        if (settings == nullptr)
            return;

        for (auto& actionId : actionIds) {
            auto key = "shortcut_" + actionId;
            if (settings->containsKey(key)) {
                auto value = settings->getValue(key);
                auto parts = juce::StringArray::fromTokens(value, ":", "");
                if (parts.size() == 2) {
                    int keyCode = parts[0].getIntValue();
                    int modifiers = parts[1].getIntValue();
                    bindings[actionId] = juce::KeyPress(keyCode, juce::ModifierKeys(modifiers), 0);
                }
            }
        }
    }

    void saveToProperties() {
        if (appProperties == nullptr)
            return;
        auto* settings = appProperties->getUserSettings();
        if (settings == nullptr)
            return;

        for (auto& actionId : actionIds) {
            auto& kp = bindings[actionId];
            auto value = juce::String(kp.getKeyCode()) + ":" + juce::String(kp.getModifiers().getRawFlags());
            settings->setValue("shortcut_" + actionId, value);
        }
        appProperties->saveIfNeeded();
        if (onBindingsChanged)
            onBindingsChanged();
    }

    juce::KeyPress getBinding(const juce::String& actionId) const {
        auto it = bindings.find(actionId);
        return it != bindings.end() ? it->second : juce::KeyPress();
    }

    juce::String getActionForKeyPress(const juce::KeyPress& key) const {
        for (auto& [actionId, binding] : bindings) {
            if (towlower(binding.getKeyCode()) == towlower(key.getKeyCode()) &&
                binding.getModifiers() == key.getModifiers())
                return actionId;
        }
        return {};
    }

    void setBinding(const juce::String& actionId, const juce::KeyPress& key) { bindings[actionId] = key; }

    bool hasConflict(const juce::String& actionId, const juce::KeyPress& key) const {
        for (auto& [otherId, binding] : bindings) {
            if (otherId != actionId && towlower(binding.getKeyCode()) == towlower(key.getKeyCode()) &&
                binding.getModifiers() == key.getModifiers())
                return true;
        }
        return false;
    }

    juce::String getConflictingAction(const juce::String& actionId, const juce::KeyPress& key) const {
        for (auto& [otherId, binding] : bindings) {
            if (otherId != actionId && towlower(binding.getKeyCode()) == towlower(key.getKeyCode()) &&
                binding.getModifiers() == key.getModifiers())
                return otherId;
        }
        return {};
    }

    void resetToDefaults() {
        bindings.clear();
        bindings["openSettings"] = juce::KeyPress(',', juce::ModifierKeys::commandModifier, 0);
        bindings["savePreset"] = juce::KeyPress('s', juce::ModifierKeys::commandModifier, 0);
        bindings["openPreset"] = juce::KeyPress('o', juce::ModifierKeys::commandModifier, 0);
        bindings["undo"] = juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0);
        bindings["redo"] =
            juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0);
    }

    static juce::String keyPressToDisplayString(const juce::KeyPress& key) {
        juce::String result;
        auto mods = key.getModifiers();

#if JUCE_MAC
        if (mods.isCommandDown())
            result += "Cmd + ";
        if (mods.isCtrlDown())
            result += "Ctrl + ";
#else
        if (mods.isCtrlDown())
            result += "Ctrl + ";
#endif
        if (mods.isAltDown())
            result += "Alt + ";
        if (mods.isShiftDown())
            result += "Shift + ";

        auto keyCode = key.getKeyCode();
        if (keyCode >= 'a' && keyCode <= 'z')
            result += juce::String::charToString(static_cast<juce::juce_wchar>(keyCode - 32));
        else if (keyCode >= 'A' && keyCode <= 'Z')
            result += juce::String::charToString(static_cast<juce::juce_wchar>(keyCode));
        else if (keyCode == ',')
            result += ",";
        else if (keyCode == '.')
            result += ".";
        else if (keyCode == '/')
            result += "/";
        else if (keyCode == ';')
            result += ";";
        else if (keyCode == '\'')
            result += "'";
        else if (keyCode == '[')
            result += "[";
        else if (keyCode == ']')
            result += "]";
        else if (keyCode == '-')
            result += "-";
        else if (keyCode == '=')
            result += "=";
        else
            result += juce::String::charToString(static_cast<juce::juce_wchar>(keyCode));

        return result;
    }

    static juce::String getActionDescription(const juce::String& actionId) {
        if (actionId == "openSettings")
            return "Open Settings";
        if (actionId == "savePreset")
            return "Save Preset";
        if (actionId == "openPreset")
            return "Open Preset";
        if (actionId == "undo")
            return "Undo";
        if (actionId == "redo")
            return "Redo";
        return actionId;
    }

    const juce::StringArray& getActionIds() const { return actionIds; }

    std::function<void()> onBindingsChanged;

private:
    std::map<juce::String, juce::KeyPress> bindings;
    juce::ApplicationProperties* appProperties = nullptr;

    juce::StringArray actionIds{"openSettings", "savePreset", "openPreset", "undo", "redo"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShortcutManager)
};
