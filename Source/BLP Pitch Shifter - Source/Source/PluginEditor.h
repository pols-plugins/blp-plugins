#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class PluginEditor final : public blp::ThemedEditor
{
public:
    explicit PluginEditor(PluginProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class Knob final : public juce::Slider
    {
    public:
        void paint(juce::Graphics&) override;
        void mouseEnter(const juce::MouseEvent& event) override { Slider::mouseEnter(event); repaint(); }
        void mouseExit(const juce::MouseEvent& event) override { Slider::mouseExit(event); repaint(); }
        void mouseDown(const juce::MouseEvent& event) override { Slider::mouseDown(event); repaint(); }
        void mouseUp(const juce::MouseEvent& event) override { Slider::mouseUp(event); repaint(); }
        void focusGained(FocusChangeType) override { repaint(); }
        void focusLost(FocusChangeType) override { repaint(); }
    };

    blp::MenuBox mode { "Input Type" };
    Knob shift, mix;
    blp::BypassButton bypass;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeBinding;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shiftBinding, mixBinding;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassBinding;
};


