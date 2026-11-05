#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour background { 0xff151518 };
const juce::Colour panel { 0xff151518 };
const juce::Colour purple { 0xffee1c25 };
const juce::Colour muted { 0xffb9b8c0 };
const juce::Colour white { 0xfff6f5f7 };

juce::Font font(float height)
{
    return juce::Font(juce::FontOptions(height));
}

void styleButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff323238));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffee1c25));
    button.setColour(juce::TextButton::textColourOffId, blp::textWhite);
    button.setColour(juce::TextButton::textColourOnId, blp::textWhite);
}
}

BitcrusherEditor::ValueControl::ValueControl(const juce::String& name, int minimum,
                                             int maximum, bool percent, bool overWaveform)
    : Slider(juce::Slider::LinearBarVertical, juce::Slider::NoTextBox),
      hasPercent(percent), isIntensity(overWaveform)
{
    setName(name);
    setTitle(name);
    setRange(minimum, maximum, 1.0);
    setSliderSnapsToMousePosition(false);
    setScrollWheelEnabled(true);
    setWantsKeyboardFocus(true);
    setMouseCursor(juce::MouseCursor::UpDownResizeCursor);

}

int BitcrusherEditor::ValueControl::getLineY()
{
    const auto position = static_cast<float>(valueToProportionOfLength(getValue()));
    return juce::roundToInt(juce::jmap(position, static_cast<float>(getHeight() - 24), 24.0f));
}

void BitcrusherEditor::ValueControl::paint(juce::Graphics& g)
{
    const auto value = getTextFromValue(getValue());
    if (isIntensity)
    {
        const auto y = getLineY();
        g.setColour(white.withAlpha(0.85f));
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(getWidth()));
        g.setFont(font(12.0f));
        blp::drawText(g, value, 8, y - 18, 50, 16, juce::Justification::centredLeft);

        const juce::Rectangle<float> pill(static_cast<float>(getWidth() / 2 - 53),
                                           static_cast<float>(y - 16), 106.0f, 32.0f);
        g.setColour(panel);
        g.fillRoundedRectangle(pill, 16.0f);
        g.setColour(white);
        g.drawRoundedRectangle(pill, 16.0f, 1.0f);
        g.setFont(font(14.0f));
        blp::drawText(g, getName(), pill.toNearestInt().translated(-5, 0), juce::Justification::centred);
        blp::paintAdjustArrows(g, pill);
        return;
    }

    constexpr float trackX = 9.0f;
    constexpr float top = 8.0f;
    const auto bottom = static_cast<float>(getHeight() - 8);
    const auto position = static_cast<float>(valueToProportionOfLength(getValue()));
    const auto thumb = juce::jmap(position, bottom, top);
    g.setColour(juce::Colour(0xff47474f));
    g.drawVerticalLine(juce::roundToInt(trackX), top, bottom);
    g.setColour(purple);
    g.drawVerticalLine(juce::roundToInt(trackX), thumb, bottom);
    g.setColour(white);
    g.drawHorizontalLine(juce::roundToInt(thumb), trackX - 3.0f, trackX + 4.0f);

    g.setColour(muted);
    g.setFont(font(12.0f));
    blp::drawText(g, getName(), 23, 5, getWidth() - 26, 18, juce::Justification::centredLeft);
    g.setColour(white);
    g.setFont(font(19.0f));
    blp::drawText(g, value, 23, 25, getWidth() - 26, 24, juce::Justification::centredLeft);
}

void BitcrusherEditor::ValueControl::resized()
{
    Slider::resized();
    setMouseDragSensitivity(juce::jmax(1, getHeight() - (isIntensity ? 48 : 16)));
}

juce::String BitcrusherEditor::ValueControl::getTextFromValue(double value)
{
    return juce::String(juce::roundToInt(value)) + (hasPercent ? "%" : "");
}

void BitcrusherEditor::Waveform::update(float amount, float depth, bool snap)
{
    if (amount != lastAmount || depth != lastDepth)
    {
        lastAmount = amount;
        lastDepth = depth;
        animating = true;
        const auto segments = 5.0f + 118.0f * std::pow(1.0f - amount, 2.1f);
        // Exaggerate vertical quantization in the preview so 8 bits is legible at this size.
        const auto halfLevels = std::exp2((depth - 1.0f) * 0.55f);
        for (size_t i = 0; i < points.size(); ++i)
        {
            const auto x = static_cast<float>(i) / static_cast<float>(points.size() - 1);
            const auto heldX = std::floor(x * segments) / segments;
            const auto position = heldX + (x - heldX) * std::max(0.0f, 1.0f - amount * 5.0f);
            const auto t = position * juce::MathConstants<float>::twoPi;
            const auto signal = 0.61f * std::sin(2.0f * t + 0.35f)
                              + 0.23f * std::sin(5.0f * t - 0.7f)
                              + 0.11f * std::sin(9.0f * t + 0.8f);
            targets[i] = (std::floor(signal * halfLevels) + 0.5f) / halfLevels;
        }
    }
    if (!animating)
        return;

    animating = false;
    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto delta = targets[i] - points[i];
        if (snap || std::abs(delta) < 0.001f)
            points[i] = targets[i];
        else
        {
            points[i] += delta * 0.29f;
            animating = true;
        }
    }

    repaint();
}

void BitcrusherEditor::Waveform::paint(juce::Graphics& g)
{
    g.setColour(panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    const auto area = getLocalBounds().toFloat().reduced(10.0f, 16.0f);
    const auto mid = area.getCentreY();
    const auto spacing = area.getWidth() / static_cast<float>(points.size() - 1);
    const auto gain = area.getHeight() * 0.47f;
    juce::Path wave;
    wave.startNewSubPath(area.getX(), mid - gain * points.front());
    for (size_t i = 1; i < points.size(); ++i)
    {
        const auto x = area.getX() + static_cast<float>(i) * spacing;
        wave.lineTo(x, mid - gain * points[i - 1]);
        wave.lineTo(x, mid - gain * points[i]);
    }
    g.setColour(purple);
    g.strokePath(wave, juce::PathStrokeType(1.55f, juce::PathStrokeType::curved));
}

BitcrusherEditor::BitcrusherEditor(BitcrusherProcessor& effect)
    : blp::ThemedEditor(effect),
      intensityAttachment(effect.parameters, "intensity", intensity),
      bitsAttachment(effect.parameters, "bits", bits),
      mixAttachment(effect.parameters, "mix", mix),
      bypassAttachment(effect.parameters, "bypass", bypass)
{
    for (auto* component : std::array<juce::Component*, 5> {
             &waveform, &intensity, &bits, &mix, &bypass })
        addAndMakeVisible(component);

    styleButton(bypass);
    waveform.update(static_cast<float>(intensity.getValue()) * 0.01f,
                    static_cast<float>(bits.getValue()), true);
    setSize(248, 270);
    startTimerHz(45);
}

void BitcrusherEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(white);
    g.setFont(font(17.0f));
    
    blp::paintHeader(g, getWidth(), "BLP Bitcrusher");
}

void BitcrusherEditor::resized()
{
    waveform.setBounds(0, 39, getWidth(), 152);
    intensity.setBounds(waveform.getBounds());
    bypass.setBounds(blp::bypassBounds(getWidth()));
    bits.setBounds(17, 200, 101, 58);
    mix.setBounds(getWidth() / 2 + 8, 200, 101, 58);
}

void BitcrusherEditor::timerCallback()
{
    waveform.update(static_cast<float>(intensity.getValue()) * 0.01f,
                    static_cast<float>(bits.getValue()), false);
}
