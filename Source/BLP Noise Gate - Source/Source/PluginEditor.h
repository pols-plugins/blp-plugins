#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class NoiseGateAudioProcessorEditor final : public blp::ThemedEditor
{
public:
    explicit NoiseGateAudioProcessorEditor (NoiseGateAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct SliderStyle final : blp::EditorLookAndFeel
    {
        void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                               juce::Slider::SliderStyle, juce::Slider&) override;
    };

    struct Control final : juce::Component
    {
        Control (juce::AudioProcessorValueTreeState&, const char* id, const char* title,
                 const char* unit, SliderStyle&, bool large = false);
        void resized() override;
        void updateValue();

        const bool large;
        const juce::String unit;
        juce::Slider slider;
        juce::Label label, value;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    SliderStyle sliderStyle;
    Control threshold, attack, release;
    blp::BypassButton bypass;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
};

