#include "PluginEditor.h"

using namespace juce;
namespace
{
const Colour background { 0xff151518 };
const Colour border { 0xff222226 };
const Colour purple { 0xffee1c25 };
const auto muted = blp::muted;

void font(Graphics& g, float size)
{
    g.setFont(FontOptions(size));
}
}

void EchoEditor::ClickMenu::paint(Graphics& g)
{
    const bool highlighted = isMouseOver() || isPopupActive() || hasKeyboardFocus(false);
    if (showBackground || highlighted)
    {
        g.setColour(highlighted ? blp::edge : border);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f);
    }

    g.setFont(FontOptions(showBackground ? 12.0f : 15.0f));
    blp::drawText(g, getText(), getLocalBounds(), Justification::centred);
}

EchoEditor::EchoEditor(EchoProcessor& processor) : blp::ThemedEditor(processor)
{
    for (auto* menu : { &division, &mode })
        menu->setColour(ComboBox::textColourId, Colours::transparentBlack);

    auto slider = [&](PaintedSlider& control, const char* name, double reset)
    {
        control.setSliderStyle(Slider::RotaryVerticalDrag);
        control.setMouseDragSensitivity(125);
        control.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        control.setDoubleClickReturnValue(true, reset);
        control.setName(name);
        control.setTooltip(String(name) + " - drag, scroll, or double-click to reset");
        control.onValueChange = [this] { repaint(); };
        addAndMakeVisible(control);
    };
    slider(feedback, "Feedback", 24);
    slider(milliseconds, "Delay Time", 200);
    slider(cutoff, "Cutoff", 12000);
    slider(mix, "Mix", 40);
    division.addItemList(EchoProcessor::divisions, 1);
    division.setName("Beat Division");
    division.setTooltip("Choose a beat division");
    mode.addItemList({ "beat", "ms" }, 1);
    mode.setName("Time Mode");
    mode.showBackground = true;
    mode.setTooltip("Choose beat sync or milliseconds");
    addAndMakeVisible(division);
    addAndMakeVisible(mode);
    addAndMakeVisible(bypass);
    mode.onChange = [this] { updateMode(); };
    auto& state = processor.state;
    feedbackBinding = std::make_unique<SliderBinding>(state, "feedback", feedback);
    msBinding = std::make_unique<SliderBinding>(state, "ms", milliseconds);
    cutoffBinding = std::make_unique<SliderBinding>(state, "cutoff", cutoff);
    mixBinding = std::make_unique<SliderBinding>(state, "mix", mix);
    divisionBinding = std::make_unique<ChoiceBinding>(state, "division", division);
    modeBinding = std::make_unique<ChoiceBinding>(state, "mode", mode);
    bypassBinding = std::make_unique<ButtonBinding>(state, "bypass", bypass);
    updateMode();
    setSize(332, 270);
    startTimerHz(30);
}

void EchoEditor::updateMode()
{
    const bool manual = mode.getSelectedId() == 2;
    division.setVisible(!manual);
    milliseconds.setVisible(manual);
    repaint();
}

void EchoEditor::timerCallback()
{
    const double motion = jmax(0.0, feedback.getValue() * 0.01 - 0.25) * 1.33;
    phase = std::fmod(phase + motion * 0.45 / 30.0, 1.0);
    if (feedback.getValue() > 25)
        repaint(0, 33, getWidth(), 153);
}

void EchoEditor::resized()
{
    feedback.setBounds(0, 34, getWidth(), 148);
    bypass.setBounds(blp::bypassBounds(getWidth()));
    division.setBounds(29, 222, 61, 24);
    milliseconds.setBounds(28, 208, 68, 44);
    mode.setBounds(95, 207, 34, 16);
    cutoff.setBounds(144, 207, 84, 46);
    mix.setBounds(232, 207, 87, 46);
}

void EchoEditor::paint(Graphics& g)
{
    g.fillAll(blp::canvas);
    g.setColour(background);
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 7);
    g.setColour(Colours::white);
    font(g, 13);
    

    const float centreX = getWidth() * 0.5f;
    const float amount = float(feedback.getValue()) * 0.01f;
    {
        Graphics::ScopedSaveState saved(g);
        g.reduceClipRegion(1, 34, getWidth() - 2, 149);
        auto ring = [&](float radius, float opacity, float thickness)
        {
            g.setColour(purple.withAlpha(opacity));
            g.drawEllipse(centreX - radius, 96.0f - radius, 2 * radius, 2 * radius, thickness);
        };
        ring(16, 0.92f, 0.85f);
        if (amount > 0)
        {
            const int count = 4 + int(amount * 5.0f);
            for (int i = 0; i < count; ++i)
            {
                const float motion = jmax(0.0f, amount - 0.25f) * 1.33f;
                const float position = std::fmod(float(i + 1) / float(count + 1) + float(amount > 0.25f ? phase : 0.0), 1.0f);
                const float radius = 16.0f + position * (58.0f + 95.0f * amount);
                const float opacity = (1.0f - position) * (0.7f + 0.3f * amount) * jmin(1.0f, amount * 5.0f);
                if (motion > 0.0f)
                    ring(radius, opacity * motion * 0.28f, 3.0f);
                ring(radius, opacity * jmin(1.0f, position * 14.0f), 0.75f);
            }
        }
    }

    const float y = 145.0f - amount * 99.0f;
    g.setColour(Colours::white);
    g.drawHorizontalLine(roundToInt(y), 1.0f, static_cast<float>(getWidth() - 1));
    font(g, 10);
    blp::drawText(g, String(roundToInt(feedback.getValue())) + " %", 6, roundToInt(y) - 15, 55, 14, Justification::centredLeft);
    const Rectangle<float> pill(centreX - 54.0f, y - 16, 108, 32);
    g.setColour(background);
    g.fillRoundedRectangle(pill, 16);
    g.setColour(Colours::white);
    g.drawRoundedRectangle(pill, 16, 0.9f);
    font(g, 14);
    blp::drawText(g, "Feedback", pill.toNearestInt().translated(-5, 0), Justification::centred);
    blp::paintAdjustArrows(g, pill);

    auto control = [&](int x, const char* title, const String& value, float level)
    {
        g.setColour(muted);
        font(g, 13);
        blp::drawText(g, title, x, 207, 100, 17, Justification::centredLeft);
        g.setColour(Colour(0xff323238));
        g.drawVerticalLine(x - 12, 213, 243);
        g.setColour(purple);
        g.drawVerticalLine(x - 12, 243 - level * 27.0f, 243);
        g.setColour(Colours::white);
        g.drawHorizontalLine(roundToInt(243 - level * 27.0f), float(x - 14), float(x - 9));
        font(g, 16);
        blp::drawText(g, value, x, 225, 102, 21, Justification::centredLeft);
    };
    const bool manual = mode.getSelectedId() == 2;
    control(35, "Time", manual ? String(roundToInt(milliseconds.getValue())) + " ms" : String(),
            manual ? float(milliseconds.getValue()) * 0.001f : 1.0f);
    control(149, "Cutoff", String(roundToInt(cutoff.getValue())) + " Hz",
            float(cutoff.valueToProportionOfLength(cutoff.getValue())));
    control(237, "Mix", String(roundToInt(mix.getValue())) + " %",
            float(mix.getValue()) * 0.01f);
    blp::paintHeader(g, getWidth(), "BLP Filter Echo +");
}

