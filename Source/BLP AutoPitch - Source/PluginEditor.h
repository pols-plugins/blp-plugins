#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class Editor final : public blp::ThemedEditor, private juce::Timer
{
public:
    explicit Editor(Processor&);
    ~Editor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    struct DialStyle final : blp::EditorLookAndFeel
    {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } dialStyle;
    Processor& processor;
    juce::Slider tuning;
    blp::MenuBox scale { "Scale" };
    blp::BypassButton bypass;
    struct KeyButton final : juce::TextButton
    {
        void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    };
    std::array<KeyButton, 12> notes;
    juce::Label readout;
    juce::AudioProcessorValueTreeState::SliderAttachment tuningBinding;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment scaleBinding;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassBinding;
    juce::ParameterAttachment rootBinding;
};

