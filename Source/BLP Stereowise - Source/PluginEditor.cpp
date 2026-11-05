#include "PluginEditor.h"

namespace
{
const juce::Colour background{0xff151518};
const juce::Colour ring{0xff323238};
const juce::Colour orange{0xffee1c25};
const juce::Colour blue{0xffee1c25};
}

juce::Label* KnobLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = blp::EditorLookAndFeel::createSliderTextBox(slider);
    label->setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return label;
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float position, float start, float end, juce::Slider&)
{
    const auto radius = static_cast<float>(juce::jmin(w, h)) * 0.5f - 5.0f;
    const auto cx = x + w * 0.5f;
    const auto cy = y + h * 0.5f;
    const auto angle = start + position * (end - start);
    const juce::Rectangle<float> bounds(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour(ring);
    g.fillEllipse(bounds.expanded(1.5f));

    juce::Path arc;
    if (position > 0.0f)
    {
        arc.addCentredArc(cx, cy, radius, radius, 0.0f, start, angle, true);
        g.setColour(orange);
        g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour(juce::Colours::white);
    g.fillEllipse(bounds.reduced(6.0f));
    const auto marker = juce::Point<float>(cx, cy).getPointOnCircumference(radius - 13.0f, angle);
    g.setColour(ring);
    g.fillEllipse(marker.x - 2.0f, marker.y - 2.0f, 4.0f, 4.0f);
}

void PluginEditor::Knob::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
        showTextBox();
    else
        Slider::mouseDown(event);
}

PluginEditor::PluginEditor(PluginProcessor& processor)
    : blp::ThemedEditor(processor),
      spreadAttachment(processor.parameters, "spread", spread),
      lowAttachment(processor.parameters, "low", low),
      highAttachment(processor.parameters, "high", high),
      bypassAttachment(processor.parameters, "bypass", bypass)
{
    setLookAndFeel(&appearance);
    for (auto* knob : { &spread, &low, &high })
    {
        knob->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 63, 19);
        knob->setColour(juce::Slider::textBoxTextColourId, blp::textWhite);
        knob->setColour(juce::Slider::textBoxBackgroundColourId, background);
        knob->setColour(juce::Slider::textBoxOutlineColourId, background);
        knob->setScrollWheelEnabled(true);
        knob->setMouseDragSensitivity(220);
        addAndMakeVisible(*knob);
        blp::showValueOnHover(*knob, *this);
    }
    spread.setDoubleClickReturnValue(true, 5.7);
    low.setDoubleClickReturnValue(true, 100.0);
    high.setDoubleClickReturnValue(true, 100.0);
    spread.setTitle("Spread");
    low.setTitle("Low");
    high.setTitle("High");
    addAndMakeVisible(bypass);
    setSize(268, 270);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel(nullptr);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(15.0f));
    
    g.setColour(blue);
    g.setFont(juce::FontOptions(13.0f));
    blp::drawText(g, "Spread", 84, 128, 100, 18, juce::Justification::centred);
    blp::drawText(g, "Low", 23, 224, 90, 18, juce::Justification::centred);
    blp::drawText(g, "High", 155, 224, 90, 18, juce::Justification::centred);
    blp::paintHeader(g, getWidth(), "BLP Stereowise");
}

void PluginEditor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    spread.setBounds(86, 48, 96, 99);
    low.setBounds(30, 155, 76, 88);
    high.setBounds(162, 155, 76, 88);
}
