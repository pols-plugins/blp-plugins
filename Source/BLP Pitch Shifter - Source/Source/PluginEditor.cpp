#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& processor)
    : blp::ThemedEditor(processor),
      bypassBinding(processor.state, "bypass", bypass)
{
    mode.addItemList({"Vocal", "General", "Percussion"}, 1);
    modeBinding = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state, "input", mode);
    mode.setTitle("Input Type");
    addAndMakeVisible(mode);
    addAndMakeVisible(bypass);

    for (auto* knob : { &shift, &mix })
    {
        knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob->setWantsKeyboardFocus(true);
        addAndMakeVisible(*knob);
    }
    shift.setTitle("Shift");
    mix.setTitle("Mix");
    shiftBinding = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state, "shift", shift);
    mixBinding = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state, "mix", mix);
    shift.textFromValueFunction = [](double value)
    {
        return (value > 0 ? "+" : "") + juce::String(int(value)) + " st";
    };
    mix.textFromValueFunction = [](double value) { return juce::String(value, 1).trimCharactersAtEnd("0").trimCharactersAtEnd(".") + "%"; };
    setSize(232, 270);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(juce::Colour(0xfff6f5f7));
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    blp::drawText(g, "Shift", 63, 158, 106, 20, juce::Justification::centred);
    blp::drawText(g, "Mix", 63, 234, 106, 20, juce::Justification::centred);
    blp::paintHeader(g, getWidth(), "BLP Pitch Shifter");
}

void PluginEditor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    mode.setBounds(19, 49, 194, 34);
    shift.setBounds(77, 98, 78, 70);
    mix.setBounds(82, 181, 68, 64);
}


void PluginEditor::Knob::paint(juce::Graphics& g)
{
    const juce::Point<float> centre { getWidth() * 0.5f, (getHeight() - 12.0f) * 0.5f };
    constexpr float start = juce::MathConstants<float>::pi * 1.22f;
    constexpr float end = juce::MathConstants<float>::pi * 2.78f;
    const auto ringRadius = centre.y - 2.0f;
    const auto faceRadius = ringRadius * 0.69f;
    const auto position = float(valueToProportionOfLength(getValue()));
    const auto angle = juce::jmap(position, start, end);
    const auto highlighted = isMouseOverOrDragging() || hasKeyboardFocus(false);

    auto drawArc = [&](float to, juce::Colour colour)
    {
        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, ringRadius, ringRadius, 0, start, to, true);
        g.setColour(colour);
        g.strokePath(arc, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    };

    drawArc(end, juce::Colour(0xff47474f));
    if (position > 0.0f)
        drawArc(angle, juce::Colour(0xffee1c25));

    g.setColour(juce::Colour(0xfff6f5f7));
    g.fillEllipse(centre.x - faceRadius, centre.y - faceRadius, 2 * faceRadius, 2 * faceRadius);
    const juce::Point<float> direction { std::sin(angle), -std::cos(angle) };
    g.setColour(juce::Colour(0xff323238));
    g.drawLine({ centre + direction * (faceRadius * 0.45f),
                 centre + direction * (faceRadius * 0.8f) }, 1.5f);

    if (highlighted)
    {
        const juce::Rectangle<float> badge(centre.x - 24.0f, getHeight() - 24.0f, 48.0f, 13.0f);
        g.setColour(juce::Colour(0xff222226));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(juce::Colours::whitesmoke);
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        blp::drawText(g, getTextFromValue(getValue()), badge.toNearestInt(), juce::Justification::centred);
    }
}
