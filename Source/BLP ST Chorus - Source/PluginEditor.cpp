#include "PluginEditor.h"

namespace
{
const auto panel = juce::Colour(0xff151518);
const auto muted = juce::Colour(0xffb9b8c0);
const auto cyan = juce::Colour(0xffee1c25);
}

ChorusEditor::ChorusEditor(ChorusProcessor& owner)
    : blp::ThemedEditor(owner), processor(owner),
      speedAttachment(processor.state, "speed", speed),
      depthAttachment(processor.state, "depth", depth),
      bypassAttachment(processor.state, "bypass", bypass)
{
    configureSlider(speed, "Speed", 60.0, 140);
    configureSlider(depth, "Depth", 100.0, 118);
    addAndMakeVisible(bypass);
    setSize(272, 270);
    startTimerHz(60);
}

void ChorusEditor::configureSlider(juce::Slider& slider, const juce::String& name,
                                   double initial, int sensitivity)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity(sensitivity);
    slider.setDoubleClickReturnValue(true, initial);
    slider.setTitle(name);
    slider.setAlpha(0.0f);
    addAndMakeVisible(slider);
}

void ChorusEditor::resized()
{
    depth.setBounds(8, 48, 256, 160);
    speed.setBounds(22, 211, 115, 52);
    bypass.setBounds(blp::bypassBounds(getWidth()));
}

void ChorusEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const double dt = juce::jlimit(0.0, 0.05, (now - lastFrame) * 0.001);
    lastFrame = now;
    phase = std::fmod(phase + dt * ChorusProcessor::rate(static_cast<float>(speed.getValue() * 0.01))
                                   * juce::MathConstants<double>::twoPi,
                      juce::MathConstants<double>::twoPi);
    repaint();
}

void ChorusEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(6.0f), 11.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(15.0f));
    

    const auto amount = static_cast<float>(depth.getValue() * 0.01);
    const float radius = 22.0f + 13.0f * amount;
    const float cy = 190.0f - 69.0f * amount;
    const float cx = 136.0f;
    if (amount == 0.0f)
    {
        g.setColour(cyan);
        g.drawEllipse(cx - radius, cy - radius, 2.0f * radius, 2.0f * radius, 0.85f);
    }
    else
    {
        for (int ring = 3; ring >= 0; --ring)
        {
            const float opacity = ring == 0 ? 0.93f
                : juce::jlimit(0.0f, 1.0f, amount * 2.0f - 0.45f * ring) * (0.46f - 0.09f * ring);
            if (opacity <= 0.0f)
                continue;
            juce::Path path;
            for (int vertex = 0; vertex < 128; ++vertex)
            {
                const double a = vertex * juce::MathConstants<double>::twoPi / 128.0;
                const float wobble = static_cast<float>(std::sin(3.0 * a - phase + ring * 1.2)
                    + 0.47 * std::sin(5.0 * a + phase + ring * 0.8));
                const float r = radius + amount * (ring * 4.3f + (1.4f + ring * 1.8f) * wobble);
                const float x = cx + r * static_cast<float>(std::cos(a));
                const float y = cy + r * static_cast<float>(std::sin(a));
                if (vertex == 0)
                    path.startNewSubPath(x, y);
                else
                    path.lineTo(x, y);
            }
            path.closeSubPath();
            g.setColour(cyan.withAlpha(opacity));
            g.strokePath(path, juce::PathStrokeType(0.85f));
        }
    }

    const int y = juce::roundToInt(184.0f - 116.0f * amount);
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawHorizontalLine(y, 7.0f, 265.0f);
    g.setFont(juce::FontOptions(11.0f));
    blp::drawText(g, juce::String(depth.getValue(), depth.getValue() == std::floor(depth.getValue()) ? 0 : 1)
                   + "%", 14, y - 17, 66, 15, juce::Justification::centredLeft);
    const juce::Rectangle<float> pill(92.0f, y - 16.0f, 88.0f, 32.0f);
    g.setColour(panel);
    g.fillRoundedRectangle(pill, 16.0f);
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(pill, 16.0f, 1.0f);
    g.setFont(juce::FontOptions(14.0f));
    blp::drawText(g, "Depth", 104, y - 13, 49, 26, juce::Justification::centred);
    juce::Path arrows;
    arrows.startNewSubPath(158, y - 1.5f);
    arrows.lineTo(161, y - 4.5f);
    arrows.lineTo(164, y - 1.5f);
    arrows.startNewSubPath(158, y + 1.5f);
    arrows.lineTo(161, y + 4.5f);
    arrows.lineTo(164, y + 1.5f);
    g.strokePath(arrows, juce::PathStrokeType(0.8f));

    g.setColour(muted);
    g.setFont(juce::FontOptions(13.0f));
    blp::drawText(g, "Speed", 44, 212, 91, 18, juce::Justification::centredLeft);
    g.setColour(cyan);
    g.drawVerticalLine(31, 231.0f, 250.0f);
    g.setColour(juce::Colours::white);
    g.drawHorizontalLine(juce::roundToInt(249 - 18 * speed.getValue() * 0.01), 29.0f, 34.0f);
    g.setFont(juce::FontOptions(17.0f));
    blp::drawText(g, juce::String(speed.getValue(), 1) + " %", 44, 231, 100, 23,
               juce::Justification::centredLeft);
    blp::paintHeader(g, getWidth(), "BLP ST Chorus");
}
