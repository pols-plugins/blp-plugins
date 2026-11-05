#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class ChorusEditor final : public blp::ThemedEditor, private juce::Timer
{
public:
    explicit ChorusEditor(ChorusProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void configureSlider(juce::Slider&, const juce::String&, double initial, int sensitivity);
    ChorusProcessor& processor;
    juce::Slider speed, depth;
    blp::BypassButton bypass;
    juce::AudioProcessorValueTreeState::SliderAttachment speedAttachment, depthAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
    double phase = 0.0;
    double lastFrame = juce::Time::getMillisecondCounterHiRes();
};
