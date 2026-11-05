#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class ChorusEditor final : public blp::ThemedEditor
{
public:
    explicit ChorusEditor(Chorus&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct KnobStyle final : blp::EditorLookAndFeel
    {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } style;
    std::array<juce::Slider, 5> knobs;
    std::array<juce::Label, 5> labels;
    blp::BypassButton bypass;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
};
