#pragma once
#include "BlpEditorTheme.h"
#include "PluginProcessor.h"

class VocalEnhancerEditor final : public blp::ThemedEditor
{
public:
    explicit VocalEnhancerEditor(VocalEnhancerProcessor&);
    ~VocalEnhancerEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class Style final : public blp::MenuLookAndFeel
    {
    public:
        Style();
        void drawPopupMenuItemWithOptions(juce::Graphics&, const juce::Rectangle<int>&, bool,
                                          const juce::PopupMenu::Item&,
                                          const juce::PopupMenu::Options&) override;
        void getIdealPopupMenuItemSizeWithOptions(const juce::String&, bool, int, int&, int&,
                                                 const juce::PopupMenu::Options&) override;
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float,
                              juce::Slider&) override;
        void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    };

    Style style;
    juce::Slider mix;
    juce::ComboBox mode;
    blp::BypassButton bypass;
    juce::Label mixLabel;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    std::optional<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
};



