#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class DriverEditor final : public blp::ThemedEditor, private juce::Thread, private juce::Timer
{
public:
    explicit DriverEditor(DriverProcessor&);
    ~DriverEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct ControlStyle : blp::EditorLookAndFeel
    {
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
        juce::Label* createSliderTextBox(juce::Slider&) override;
        juce::Slider::SliderLayout getSliderLayout(juce::Slider&) override;
        void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                              juce::Slider::SliderStyle, juce::Slider&) override;
    };

    void run() override;
    void timerCallback() override;
    void updateDrivePosition();
    int driveLineY() const;

    DriverProcessor& processor;
    ControlStyle style;
    juce::Component controls;
    juce::Slider drive, tone, volume;
    blp::BypassButton bypass;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> driveBinding, toneBinding, volumeBinding;
    std::unique_ptr<ButtonAttachment> bypassBinding;

    static constexpr int sampleCount = 576;
    juce::CriticalSection previewLock;
    std::array<float, sampleCount> preview{}, displayed{};
    bool previewChanged = false;
};
