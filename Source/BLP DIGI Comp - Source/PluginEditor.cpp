#include "PluginEditor.h"

CompressorEditor::CompressorEditor(Compressor& processor) : blp::ThemedEditor(processor)
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.setLookAndFeel(&style);
        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setPopupDisplayEnabled(true, true, this);
        knob.setVelocityModeParameters(0.45, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        knob.setTitle(Compressor::names[i]);
        addAndMakeVisible(knob);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, Compressor::ids[i], knob);
        knob.setDoubleClickReturnValue(true, Compressor::defaults[i]);
        labels[i].setText(Compressor::names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setFont(juce::FontOptions(11.0f));
        labels[i].setColour(juce::Label::textColourId, blp::textWhite);
        addAndMakeVisible(labels[i]);
    }
    addAndMakeVisible(bypass);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.state, Compressor::ids[5], bypass);
    setSize(286, 270);
}

void CompressorEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(juce::Colour(0xfff6f5f7));
    g.setFont(juce::FontOptions(16.f));
    
    blp::paintHeader(g, getWidth(), "BLP DIGI Comp");
}

void CompressorEditor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    constexpr std::array centres { 55, 143, 231, 99, 187 };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int top = i < 3 ? 56 : 160;
        knobs[i].setBounds(centres[i] - 32, top, 64, 64);
        labels[i].setBounds(centres[i] - 43, top + 62, 86, 18);
    }
}

void CompressorEditor::KnobStyle::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                                   float value, float start, float end, juce::Slider&)
{
    const juce::Point<float> centre { x + w * 0.5f, y + h * 0.5f };
    constexpr float radius = 28.f;
    const float angle = start + value * (end - start);
    const auto drawArc = [&](float to, juce::Colour colour)
    {
        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, radius, radius, 0.f, start, to, true);
        g.setColour(colour);
        g.strokePath(arc, juce::PathStrokeType(2.f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    };
    drawArc(end, juce::Colour(0xff47474f));
    if (value > 0.f)
        drawArc(angle, juce::Colour(0xffee1c25));

    const juce::Rectangle<float> face { centre.x - 21.f, centre.y - 21.f, 42.f, 42.f };
    g.setColour(juce::Colour(0xfff6f5f7));
    g.fillEllipse(face);
    g.setColour(juce::Colour(0xff151518));
    g.drawEllipse(face, 1.f);
    g.drawLine({ centre.getPointOnCircumference(9.f, angle),
                 centre.getPointOnCircumference(16.f, angle) }, 1.8f);
}
