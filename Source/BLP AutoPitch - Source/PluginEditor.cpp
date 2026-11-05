#include "PluginEditor.h"

namespace
{
const juce::Colour background {0xff222226}, panel {0xff323238}, border {0xff47474f};
const juce::Colour blue {0xffee1c25}, lightBlue {0xffee1c25}, muted {0xfff6f5f7};
}

Editor::Editor(Processor& p)
    : blp::ThemedEditor(p), processor(p),
      tuningBinding(p.params, "tuning", tuning), scaleBinding(p.params, "scale", scale),
      bypassBinding(p.params, "bypass", bypass),
      rootBinding(*p.params.getParameter("root"), [this](float note)
      {
          for (int i = 0; i < int(notes.size()); ++i)
              notes[i].setToggleState(i == int(note), juce::dontSendNotification);
      }, nullptr)
{
    tuning.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    tuning.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    tuning.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
    tuning.setLookAndFeel(&dialStyle);
    tuning.setTitle("Pitch correction");
    tuning.setDoubleClickReturnValue(true, 100);
    scale.addItemList(Processor::scales, 1);
    scale.setSelectedItemIndex(int(p.params.getRawParameterValue("scale")->load()), juce::dontSendNotification);
    scale.setTitle("Scale");
    for (auto* component : {static_cast<juce::Component*>(&tuning), static_cast<juce::Component*>(&scale),
                            static_cast<juce::Component*>(&bypass),
                            static_cast<juce::Component*>(&readout)})
        addAndMakeVisible(component);

    auto styleButton = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, panel);
        b.setColour(juce::TextButton::buttonOnColourId, blue);
        b.setColour(juce::TextButton::textColourOffId, blp::textWhite);
        b.setColour(juce::TextButton::textColourOnId, blp::textWhite);
    };
    styleButton(bypass);
    for (int i = 0; i < int(notes.size()); ++i)
    {
        auto& note = notes[i];
        note.setButtonText(Processor::keys[i]);
        note.onClick = [this, i] { rootBinding.setValueAsCompleteGesture(float(i)); };
        addAndMakeVisible(note);
    }
    rootBinding.sendInitialUpdate();
    readout.setColour(juce::Label::textColourId, blp::textWhite);
    readout.setJustificationType(juce::Justification::centred);
    readout.setFont(juce::FontOptions(15.0f));
    readout.setInterceptsMouseClicks(false, false);
    setSize(552, 270);
    timerCallback();
    startTimerHz(30);
}

Editor::~Editor() { tuning.setLookAndFeel(nullptr); }

void Editor::timerCallback()
{
    const auto value = juce::roundToInt(tuning.getValue());
    readout.setText(value == 100 ? juce::String("Heaviest") : juce::String(value) + "%", juce::dontSendNotification);
    readout.setVisible(tuning.isMouseOverOrDragging());
    if (value > 0) tuning.repaint();
}

void Editor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.fillAll(background);
    blp::paintHeader(g, getWidth(), "BLP AutoPitch");
}

void Editor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    tuning.setBounds(6, 51, 228, 207);
    constexpr int firstKeyX = 247;
    constexpr int keyWidth = 39;
    constexpr int keyStep = 42;
    constexpr int noteRowWidth = 6 * keyStep + keyWidth;
    constexpr int scaleWidth = 191;
    scale.setBounds(firstKeyX + (noteRowWidth - scaleWidth) / 2, 95, scaleWidth, 34);
    constexpr int whiteKeys[] {0, 2, 4, 5, 7, 9, 11};
    constexpr int blackKeys[] {1, 3, 6, 8, 10};
    constexpr int gaps[] {0, 1, 3, 4, 5};
    for (int i = 0; i < 7; ++i)
        notes[whiteKeys[i]].setBounds(firstKeyX + keyStep * i, 202, keyWidth, keyWidth);
    for (int i = 0; i < 5; ++i)
        notes[blackKeys[i]].setBounds(firstKeyX + keyStep / 2 + keyStep * gaps[i], 157, keyWidth, keyWidth);
    readout.setBounds(6, 245, 228, 21);
}

void Editor::DialStyle::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                         float position, float start, float end, juce::Slider&)
{
    const float cx = float(x) + float(w) / 2, cy = float(y) + float(h) / 2;
    const float r = float(std::min(w, h)) * 0.43f;
    constexpr int segments = 37;
    const float phase = float(juce::Time::getMillisecondCounterHiRes() * 0.008);
    const float strength = position * position;
    for (int i = 0; i < segments; ++i)
    {
        const float fraction = float(i) / float(segments - 1);
        const float a = start + (end - start) * fraction;
        const bool active = fraction <= position && position > 0.0f;
        const float flicker = 0.5f + 0.3f * std::sin(phase + i * 0.9f)
                            + 0.2f * std::cos(phase * 1.8f - i * 1.4f);
        const float tip = r + (active ? 6.0f * strength * flicker : 0.0f);
        g.setColour(active ? lightBlue.withAlpha(1.0f - 0.26f * strength * (1.0f - flicker)) : border);
        g.drawLine(cx + tip * std::sin(a), cy - tip * std::cos(a),
                   cx + (r - 10) * std::sin(a), cy - (r - 10) * std::cos(a), 2.5f);
    }
    const auto face = juce::Rectangle<float>(2 * (r - 18), 2 * (r - 18)).withCentre({cx, cy});
    g.setColour(panel);
    g.fillEllipse(face);
    g.setColour(border);
    g.drawEllipse(face, 1);
    g.setColour(juce::Colours::white);
    const auto angle = start + (end - start) * position;
    const auto radius = face.getWidth() * 0.5f;
    g.drawLine(cx + (radius - 17) * std::sin(angle), cy - (radius - 17) * std::cos(angle),
               cx + (radius - 5) * std::sin(angle), cy - (radius - 5) * std::cos(angle), 2.0f);
}


void Editor::KeyButton::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    const auto box = getLocalBounds().toFloat().reduced(0.75f);
    const auto fill = getToggleState() ? blue : panel;
    g.setColour(highlighted || down ? fill.brighter(0.25f) : fill);
    g.fillRoundedRectangle(box, 3);
    g.setColour(getToggleState() ? lightBlue : muted);
    g.drawRoundedRectangle(box, 3, 1);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(17.0f));
    const auto label = getButtonText();
    if (label.containsChar('/'))
    {
        blp::drawText(g, label.upToFirstOccurrenceOf("/", false, false), 2, 2, 28, 17, juce::Justification::centred);
        blp::drawText(g, label.fromFirstOccurrenceOf("/", false, false), 9, 19, 28, 17, juce::Justification::centred);
    }
    else
        blp::drawText(g, label, getLocalBounds(), juce::Justification::centred);
}

