#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class KnobLookAndFeel final : public blp::EditorLookAndFeel
{
public:
    juce::Label* createSliderTextBox(juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class PluginEditor final : public blp::ThemedEditor
{
public:
    explicit PluginEditor(PluginProcessor&);
    ~PluginEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob : juce::Slider
    {
        void mouseDown(const juce::MouseEvent&) override;
    };

    KnobLookAndFeel appearance;
    Knob spread, low, high;
    blp::BypassButton bypass;
    juce::AudioProcessorValueTreeState::SliderAttachment spreadAttachment, lowAttachment, highAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
};

