#pragma once
#include "BlpEditorTheme.h"

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

class BitcrusherEditor final : public blp::ThemedEditor, private juce::Timer
{
public:
    explicit BitcrusherEditor(BitcrusherProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class ValueControl final : public juce::Slider
    {
    public:
        ValueControl(const juce::String& name, int minimum, int maximum, bool percent, bool overWaveform = false);
        int getLineY();
        void paint(juce::Graphics&) override;
        void resized() override;
        juce::String getTextFromValue(double) override;

    private:
        bool hasPercent;
        bool isIntensity;
    };

    class Waveform final : public juce::Component
    {
    public:
        void update(float intensity, float bits, bool snap);
        void paint(juce::Graphics&) override;

    private:
        std::array<float, 512> points {}, targets {};
        float lastAmount = -1.0f;
        float lastDepth = -1.0f;
        bool animating = false;
    };

    void timerCallback() override;

    Waveform waveform;
    ValueControl intensity { "Intensity", 0, 100, true, true };
    ValueControl bits { "Bits", 1, 16, false };
    ValueControl mix { "Mix", 0, 100, true };
    blp::BypassButton bypass;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttachment intensityAttachment, bitsAttachment, mixAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
};
