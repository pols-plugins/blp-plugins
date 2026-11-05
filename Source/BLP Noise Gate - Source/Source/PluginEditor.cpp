#include "PluginEditor.h"

namespace
{
    const juce::Colour background (0xff151518);
    const juce::Colour accent (0xffee1c25);
    const juce::Colour muted (0xffb9b8c0);
}

void NoiseGateAudioProcessorEditor::SliderStyle::drawLinearSlider (
    juce::Graphics& g, int x, int y, int width, int height, float position, float, float,
    juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto centre = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    if (slider.getName() == "Threshold")
    {
        g.setColour (juce::Colours::white);
        juce::Path wave;
        for (int offset = 0; offset <= width; ++offset)
        {
            const auto phase = static_cast<float> (offset) / static_cast<float> (width);
            const auto waveY = static_cast<float> (y) + static_cast<float> (height)
                * (0.5f - 0.44f * std::sin (phase * juce::MathConstants<float>::twoPi * 2.0f));
            const auto waveX = static_cast<float> (x + offset);
            if (offset == 0)
                wave.startNewSubPath (waveX, waveY);
            else
                wave.lineTo (waveX, waveY);
        }

        g.setColour (juce::Colours::white.withAlpha (0.30f));
        g.strokePath (wave, juce::PathStrokeType (1.0f));
        {
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (x, y, width, juce::jmax (0, juce::roundToInt (position) - y));
            g.setColour (accent);
            g.strokePath (wave, juce::PathStrokeType (1.0f));
        }

        g.setColour (juce::Colours::white);
        g.drawHorizontalLine (juce::roundToInt (position), static_cast<float> (x),
                              static_cast<float> (x + width));
        const juce::Rectangle<float> pill (centre - 57.0f, position - 15.0f, 114.0f, 30.0f);
        g.setColour (background);
        g.fillRoundedRectangle (pill, 15.0f);
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (pill, 15.0f, 1.0f);
        g.setFont (juce::FontOptions (14.0f));
        blp::drawText(g, "Threshold", pill.translated(-5.0f, 0.0f), juce::Justification::centred);
        blp::paintAdjustArrows(g, pill);
        g.setFont (juce::FontOptions (12.0f));
        blp::drawText(g, juce::String (slider.getValue(), 1) + " dB",
                    juce::Rectangle<float> (static_cast<float> (x + 4),
                        juce::jmax (0.0f, position - 19.0f), 80.0f, 18.0f),
                    juce::Justification::centredLeft);
        return;
    }

    const auto bottom = static_cast<float>(y + height);
    g.setColour(juce::Colour(0xff323238));
    g.fillRect(centre - 1.0f, static_cast<float>(y), 2.0f, static_cast<float>(height));
    g.setColour(accent);
    g.fillRect(centre - 1.0f, position, 2.0f, juce::jmax(0.0f, bottom - position));
    g.fillRect(centre - 8.0f, position - 1.0f, 16.0f, 2.0f);
}

NoiseGateAudioProcessorEditor::Control::Control (
    juce::AudioProcessorValueTreeState& state, const char* id, const char* title,
    const char* suffix, SliderStyle& style, bool isLarge)
    : large (isLarge), unit (suffix)
{
    slider.setName (title);
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setLookAndFeel (&style);
    addAndMakeVisible (slider);

    label.setText (title, juce::dontSendNotification);
    label.setColour (juce::Label::textColourId, blp::textWhite);
    value.setColour (juce::Label::textColourId, blp::textWhite);
    value.setFont (juce::FontOptions (15.0f));
    if (!large)
        addAndMakeVisible (label);
    if (!large) addAndMakeVisible (value);
    value.setInterceptsMouseClicks (false, false);

    slider.onValueChange = [this] { updateValue(); };
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
    updateValue();
}

void NoiseGateAudioProcessorEditor::Control::updateValue()
{
    const auto amount = slider.getValue();
    value.setText (juce::String (amount, large || amount != std::round (amount) ? 1 : 0)
                       + " " + unit, juce::dontSendNotification);
}

void NoiseGateAudioProcessorEditor::Control::resized()
{
    auto area = getLocalBounds();
    if (large)
    {
        slider.setBounds (area);

        return;
    }

    slider.setBounds (area.removeFromLeft (26));
    label.setBounds (area.removeFromTop (26));
    value.setBounds (area.removeFromTop (24));
}

NoiseGateAudioProcessorEditor::NoiseGateAudioProcessorEditor (NoiseGateAudioProcessor& p)
    : blp::ThemedEditor (&p),
      threshold (p.parameters, "threshold", "Threshold", "dB", sliderStyle, true),
      attack (p.parameters, "attack", "Attack", "ms", sliderStyle),
      release (p.parameters, "release", "Release", "ms", sliderStyle),
      bypassAttachment (p.parameters, "bypass", bypass)
{
    for (auto* control : { &threshold, &attack, &release })
        addAndMakeVisible (*control);
    bypass.setColour (juce::ToggleButton::textColourId, blp::textWhite);
    addAndMakeVisible (bypass);
    setSize(292, 270);
}

void NoiseGateAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (14.0f));
    
    g.setColour (juce::Colour (0xff222226));
    g.fillRect (0, 212, getWidth(), getHeight() - 212);
    blp::paintHeader(g, getWidth(), "BLP Noise Gate");
}

void NoiseGateAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    bypass.setBounds(blp::bypassBounds(getWidth()));
    area.removeFromTop(34);
    threshold.setBounds (area.removeFromTop (170).reduced (8, 0));
    area.removeFromTop (8);
    area.reduce (9, 3);
    const auto width = area.getWidth() / 2;
    attack.setBounds (area.removeFromLeft (width).withTrimmedLeft (8));
    release.setBounds (area);
}
