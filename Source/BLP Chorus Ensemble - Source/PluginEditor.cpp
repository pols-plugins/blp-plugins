#include "PluginEditor.h"

ChorusEditor::ChorusEditor(Chorus& processor) : blp::ThemedEditor(processor)
{
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.setLookAndFeel(&style);
        knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setPopupDisplayEnabled(true, true, this);
        knob.setVelocityModeParameters(0.45, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        knob.setTitle(Chorus::names[i]);
        addAndMakeVisible(knob);
        attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, Chorus::ids[i], knob);
        knob.setDoubleClickReturnValue(true, Chorus::defaults[i]);
        labels[i].setText(Chorus::names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, blp::textWhite);
        addAndMakeVisible(labels[i]);
    }
    addAndMakeVisible(bypass);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.state, Chorus::ids[5], bypass);
    setSize(388, 270);
}

void ChorusEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(juce::Colour(0x4034343b));
    for (int i = 0; i < 8; ++i)
    {
        const float offset = static_cast<float>(i) * 7.f;
        g.drawEllipse(-111.f + offset, 222.f + offset, 365.f, 248.f, 1.f);
        g.drawEllipse(155.f + offset, 238.f + offset, 330.f, 205.f, 1.f);
    }
    g.setColour(juce::Colour(0xff323238));
    g.setFont(juce::FontOptions(19.f));
    
    blp::paintHeader(g, getWidth(), "BLP Chorus Ensemble");
}

void ChorusEditor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    constexpr std::array centres { 113, 275, 75, 194, 313 };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int size = i < 2 ? 100 : 76;
        const int top = i < 2 ? 42 : 160;
        knobs[i].setBounds(centres[i] - size / 2, top, size, size);
        labels[i].setBounds(centres[i] - 44, top + size, 88, 20);
    }
}

void ChorusEditor::KnobStyle::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                               float value, float start, float end, juce::Slider&)
{
    const auto centre = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
        static_cast<float>(w), static_cast<float>(h)).getCentre();
    const float radius = static_cast<float>(std::min(w, h)) * 0.42f;
    const float angle = start + value * (end - start);
    for (int tick = 0; tick <= 10; ++tick)
    {
        const float fraction = static_cast<float>(tick) / 10.f;
        const auto point = centre.getPointOnCircumference(radius, start + fraction * (end - start));
        g.setColour(juce::Colour(fraction <= value ? 0xffee1c25 : 0xff494952));
        g.fillEllipse(point.x - 1.5f, point.y - 1.5f, 3.f, 3.f);
    }
    juce::Path drop;
    drop.startNewSubPath(0.f, -0.82f);
    drop.cubicTo(0.25f, -0.73f, 0.66f, -0.14f, 0.66f, 0.17f);
    drop.cubicTo(0.66f, 0.89f, -0.66f, 0.89f, -0.66f, 0.17f);
    drop.cubicTo(-0.66f, -0.14f, -0.25f, -0.73f, 0.f, -0.82f);
    drop.closeSubPath();
    drop.applyTransform(juce::AffineTransform::scale(radius).rotated(angle).translated(centre));
    g.setColour(juce::Colour(0xff898891));
    g.strokePath(drop, juce::PathStrokeType(1.2f));
    g.setColour(juce::Colours::white);
    g.fillPath(drop);
}
