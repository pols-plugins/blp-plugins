#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class EchoEditor final : public blp::ThemedEditor, private juce::Timer
{
public:
    explicit EchoEditor(EchoProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct PaintedSlider final : juce::Slider
    {
        void paint(juce::Graphics&) override {}
    };

    void timerCallback() override;
    void updateMode();
    using SliderBinding = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ChoiceBinding = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonBinding = juce::AudioProcessorValueTreeState::ButtonAttachment;
    struct ClickMenu final : juce::ComboBox
    {
        bool showBackground = false;
        void paint(juce::Graphics& g) override;
    };

    PaintedSlider feedback, milliseconds, cutoff, mix;
    ClickMenu division, mode;
    blp::BypassButton bypass;
    std::unique_ptr<SliderBinding> feedbackBinding, msBinding, cutoffBinding, mixBinding;
    std::unique_ptr<ChoiceBinding> divisionBinding, modeBinding;
    std::unique_ptr<ButtonBinding> bypassBinding;
    double phase = 0.0;
};
