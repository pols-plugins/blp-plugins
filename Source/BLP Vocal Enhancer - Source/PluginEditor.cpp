#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xff222226 };
const juce::Colour panel { 0xff323238 };
const juce::Colour accent { 0xffee1c25 };
const juce::Colour muted { 0xffb9b8c0 };
constexpr auto pi = juce::MathConstants<float>::pi;
}

VocalEnhancerEditor::Style::Style()
{
    setColour(juce::ComboBox::textColourId, blp::textWhite);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, blp::textWhite);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff47474f));
    setColour(juce::PopupMenu::highlightedTextColourId, blp::textWhite);
}

void VocalEnhancerEditor::Style::getIdealPopupMenuItemSizeWithOptions(
    const juce::String&, bool, int, int& width, int& height, const juce::PopupMenu::Options&)
{
    width = 224;
    height = 42;
}

void VocalEnhancerEditor::Style::drawPopupMenuItemWithOptions(
    juce::Graphics& g, const juce::Rectangle<int>& area, bool highlighted,
    const juce::PopupMenu::Item& item, const juce::PopupMenu::Options&)
{
    static constexpr std::array descriptions {
        "Adds richness and depth.",
        "Balanced, natural enhancement.",
        "Crisp highs, punchy lows.",
        "Moderate boosts to highs and lows."
    };
    if (highlighted)
    {
        g.setColour(juce::Colour(0xff47474f));
        g.fillRect(area);
    }

    auto textArea = area.reduced(12, 2);
    auto titleArea = highlighted ? textArea.removeFromTop(20) : textArea;
    g.setColour(item.isTicked ? accent : juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f));
    blp::drawText(g, item.text, titleArea, juce::Justification::centredLeft);

    if (highlighted && item.itemID > 0 && item.itemID <= static_cast<int>(descriptions.size()))
    {
        g.setColour(juce::Colour(0xfff6f5f7));
        g.setFont(juce::FontOptions(12.0f));
        blp::drawText(g, descriptions[static_cast<size_t>(item.itemID - 1)], textArea,
                   juce::Justification::centredLeft);
    }
}

void VocalEnhancerEditor::Style::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                                 float position, float start, float end, juce::Slider&)
{
    const auto centre = juce::Rectangle<int>(x, y, w, h).toFloat().getCentre();
    const auto radius = static_cast<float>(juce::jmin(w, h)) * 0.5f - 7.0f;
    const auto angle = juce::jmap(position, start, end);
    auto circle = [centre](float r) { return juce::Rectangle<float>(2 * r, 2 * r).withCentre(centre); };

    g.setColour(juce::Colour(0xff151518));
    g.fillEllipse(circle(radius + 4.0f));
    juce::Path arc;
    if (position > 0.0f)
        arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, start, angle, true);
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded));
    g.setColour(juce::Colour(0xfff6f5f7));
    g.fillEllipse(circle(radius - 7.0f));
    const juce::Point<float> direction { std::sin(angle), -std::cos(angle) };
    g.setColour(background);
    g.drawLine({ centre + direction * (radius - 25.0f), centre + direction * (radius - 12.0f) }, 2.0f);
}

void VocalEnhancerEditor::Style::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                                  bool hovered, bool pressed)
{
    const auto bounds = button.getLocalBounds();
    if (hovered || pressed)
    {
        g.setColour(panel);
        g.fillRoundedRectangle(bounds.toFloat(), 5.0f);
    }
    g.setColour(button.getToggleState() ? muted : accent);
    g.setFont(juce::FontOptions(13.0f));
    blp::drawText(g, button.getButtonText(), bounds, juce::Justification::centred);
    if (button.hasKeyboardFocus(false))
    {
        g.setColour(muted);
        g.drawRoundedRectangle(button.getLocalBounds().toFloat().reduced(1.0f), 5.0f, 1.0f);
    }
}

VocalEnhancerEditor::VocalEnhancerEditor(VocalEnhancerProcessor& processor)
    : blp::ThemedEditor(processor),
      mixAttachment(processor.parameters, "mix", mix),
      bypassAttachment(processor.parameters, "bypass", bypass)
{
    setLookAndFeel(&style);
    mix.setTitle("Mix");
    mix.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mix.setRotaryParameters(1.25f * pi, 2.75f * pi, true);
    mix.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    mix.setDoubleClickReturnValue(true, 50.0);
    mix.setVelocityModeParameters(0.35, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
    mix.setScrollWheelEnabled(true);
    mix.textFromValueFunction = [](double value) { return juce::String(value, 0) + "%"; };
    mix.setPopupDisplayEnabled(true, true, this);

    mode.setTitle("Mode");
    const auto* choices = dynamic_cast<juce::AudioParameterChoice*>(processor.parameters.getParameter("mode"));
    mode.addItemList(choices->choices, 1);
    modeAttachment.emplace(processor.parameters, "mode", mode);

    

    bypass.setTitle("Bypass");
    bypass.setTooltip("Bypass");
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setColour(juce::Label::textColourId, blp::textWhite);
    for (auto* control : std::array<juce::Component*, 4> { &mix, &mixLabel, &mode, &bypass })
        addAndMakeVisible(control);
    setSize(214, 270);
}

VocalEnhancerEditor::~VocalEnhancerEditor()
{
    setLookAndFeel(nullptr);
}

void VocalEnhancerEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f));
    
    blp::paintHeader(g, getWidth(), "BLP Vocal Enhancer");
}

void VocalEnhancerEditor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    mix.setBounds((getWidth() - 128) / 2, 48, 128, 128);
    mixLabel.setBounds(0, 176, getWidth(), 24);
    mode.setBounds((getWidth() - 176) / 2, 222, 176, 34);
}



