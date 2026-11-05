#include "PluginEditor.h"

namespace
{
const juce::Colour background{0xff151518};
const juce::Colour accent{0xffee1c25};
const juce::Colour muted{0xffb9b8c0};
constexpr float editorScale = 1.1f;
const juce::Rectangle<int> plot{0, 31, 240, 135};

int lineY(double driveValue)
{
    return 119 - juce::roundToInt(driveValue * 3.9);
}
}

juce::Label* DriverEditor::ControlStyle::createSliderTextBox(juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
    const auto fontSize = slider.getSliderStyle() == juce::Slider::RotaryVerticalDrag ? 11.0f : 16.0f;
    label->setFont(juce::Font(juce::FontOptions(fontSize)));
    label->setJustificationType(juce::Justification::centredLeft);
    label->setBorderSize(juce::BorderSize<int>(0));
    label->setInterceptsMouseClicks(false, true);
    return label;
}

juce::Slider::SliderLayout DriverEditor::ControlStyle::getSliderLayout(juce::Slider& slider)
{
    if (slider.getSliderStyle() == juce::Slider::RotaryVerticalDrag)
        return {slider.getLocalBounds(), {6, lineY(slider.getValue()) - plot.getY() - 19, 47, 18}};

    return {{7, 10, 8, 29}, {24, 25, slider.getWidth() - 24, 23}};
}

void DriverEditor::ControlStyle::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderY, float, float, juce::Slider::SliderStyle,
                                                  juce::Slider&)
{
    const float railX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    g.setColour(juce::Colour(0xff47474f));
    g.drawVerticalLine(juce::roundToInt(railX), static_cast<float>(y), static_cast<float>(y + height));
    g.setColour(accent);
    g.drawVerticalLine(juce::roundToInt(railX), sliderY, static_cast<float>(y + height));
    g.setColour(juce::Colours::white);
    g.fillRect(railX - 3.0f, sliderY - 0.5f, 7.0f, 1.0f);
}

DriverEditor::DriverEditor(DriverProcessor& owner)
    : blp::ThemedEditor(owner), Thread("BLP Driver preview"), processor(owner)
{
    addAndMakeVisible(controls);
    controls.setBounds(0, 0, 240, 248);
    controls.setTransform(juce::AffineTransform::scale(editorScale));

    for (auto* control : {&drive, &tone, &volume})
    {
        control->setLookAndFeel(&style);
        control->setSliderStyle(control == &drive ? juce::Slider::RotaryVerticalDrag : juce::Slider::LinearVertical);
        control->setMouseDragSensitivity(135);
        control->setScrollWheelEnabled(false);
        control->setTextBoxStyle(juce::Slider::TextBoxRight, true, control == &tone ? 82 : 54, 28);
        control->setColour(juce::Slider::textBoxTextColourId, blp::textWhite);
        control->setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        control->setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        control->setColour(juce::Slider::textBoxHighlightColourId, accent.withAlpha(0.5f));
        controls.addAndMakeVisible(*control);
    }
    drive.setTitle("Drive");
    tone.setTitle("Tone");
    volume.setTitle("Volume");
    driveBinding = std::make_unique<SliderAttachment>(owner.state, "drive", drive);
    toneBinding = std::make_unique<SliderAttachment>(owner.state, "tone", tone);
    volumeBinding = std::make_unique<SliderAttachment>(owner.state, "volume", volume);
    drive.setNumDecimalPlacesToDisplay(1);
    tone.setNumDecimalPlacesToDisplay(0);
    volume.setNumDecimalPlacesToDisplay(1);
    tone.setTextValueSuffix({});
    tone.textFromValueFunction = [](double value) { return juce::String(juce::roundToInt(value)) + " Hz"; };
    tone.updateText();

    addAndMakeVisible(bypass);
    bypassBinding = std::make_unique<ButtonAttachment>(owner.state, "bypass", bypass);

    drive.onValueChange = [this] { updateDrivePosition(); };
    setSize(264, 270);
    startThread();
    startTimerHz(30);
}

DriverEditor::~DriverEditor()
{
    stopTimer();
    stopThread(-1);
    for (auto* control : {&drive, &tone, &volume})
        control->setLookAndFeel(nullptr);
}

void DriverEditor::run()
{
    auto current = processor.readSettings();
    bool firstFrame = true;
    while (!threadShouldExit())
    {
        const auto target = processor.readSettings();
        const auto motion = juce::jlimit(0.0f, 1.0f, (target.drive - 3.5f) / 6.5f) * (1.0f - target.bypass);
        if (motion == 0.0f && !firstFrame && current.drive == target.drive && current.tone == target.tone
            && current.volume == target.volume && current.bypass == target.bypass)
        {
            wait(24);
            continue;
        }
        firstFrame = false;

        const auto smooth = [](float from, float to)
        {
            return std::abs(to - from) < 0.001f ? to : from + 0.38f * (to - from);
        };
        current.drive = smooth(current.drive, target.drive);
        current.tone = smooth(current.tone, target.tone);
        current.volume = smooth(current.volume, target.volume);
        current.bypass = smooth(current.bypass, target.bypass);

        constexpr float previewRate = 24000.0f;
        constexpr int period = 144;
        const auto amount = juce::jmin(1.0f, current.drive / 3.5f);
        const auto motionPhase = float(juce::Time::getMillisecondCounterHiRes() * 0.005);
        const auto inputGain = DriverProcessor::Signal::inputGain(current.drive);
        const auto coefficient = DriverProcessor::Signal::toneCoefficient(current.tone, previewRate);
        const auto gain = juce::Decibels::decibelsToGain(current.volume);
        DriverProcessor::Signal signal;
        std::array<float, sampleCount> next{};
        for (int i = -period * 4; i < sampleCount; ++i)
        {
            const auto phase = juce::MathConstants<float>::twoPi * static_cast<float>(i) / period;
            const auto texture = 0.028f * amount * (std::sin(11.0f * phase) + 0.5f * std::sin(23.0f * phase));
            const auto dry = 0.67f * std::sin(phase) + texture;
            const auto wet = signal.process(dry, amount, inputGain, coefficient, gain);
            const auto ripple = motion * motion * (0.08f * std::sin(9.0f * phase + motionPhase)
                + 0.045f * std::sin(17.0f * phase - motionPhase * 1.7f));
            const auto distorted = juce::jlimit(-1.1f, 1.1f, wet * (1.0f + 0.23f * motion) + ripple);
            if (i >= 0)
                next[static_cast<size_t>(i)] = distorted + current.bypass * (dry - distorted);
        }
        {
            const juce::ScopedLock lock(previewLock);
            previewChanged = previewChanged || next != preview;
            preview = next;
        }
        wait(24);
    }
}

void DriverEditor::timerCallback()
{
    {
        const juce::ScopedLock lock(previewLock);
        if (!previewChanged)
            return;
        displayed = preview;
        previewChanged = false;
    }
    repaint();
}

int DriverEditor::driveLineY() const
{
    return lineY(drive.getValue());
}

void DriverEditor::updateDrivePosition()
{
    drive.resized();
    repaint();
}

void DriverEditor::resized()
{
    bypass.setBounds(blp::bypassBounds(getWidth()));
    drive.setBounds(plot);
    tone.setBounds(12, 179, 108, 48);
    volume.setBounds(124, 179, 108, 48);
    updateDrivePosition();
}

void DriverEditor::paint(juce::Graphics& g)
{
    g.fillAll(blp::canvas);
    g.saveState();
    g.addTransform(juce::AffineTransform::scale(editorScale));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(15.0f)));
    

    const auto centre = driveLineY();
    g.setColour(juce::Colour(0xffb9b8c0));
    g.drawHorizontalLine(centre, 0.0f, 240.0f);
    juce::Path wave;
    for (int i = 0; i < sampleCount / 2; ++i)
    {
        const auto x = 240.0f * static_cast<float>(i) / (sampleCount / 2 - 1);
        const auto y = static_cast<float>(centre) - 15.0f * displayed[static_cast<size_t>(i)];
        if (i == 0)
            wave.startNewSubPath(x, y);
        else
            wave.lineTo(x, y);
    }
    {
        const juce::Graphics::ScopedSaveState clip(g);
        g.reduceClipRegion(plot);
        g.setColour(accent);
        g.strokePath(wave, juce::PathStrokeType(0.9f, juce::PathStrokeType::curved));
    }

    const juce::Rectangle<float> pill{81.0f, static_cast<float>(centre) - 15.5f, 78.0f, 31.0f};
    g.setColour(background);
    g.fillRoundedRectangle(pill, 15.5f);
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(pill.reduced(0.5f), 15.0f, 1.0f);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    blp::drawText(g, "Drive", pill.translated(-5.0f, 0.0f), juce::Justification::centred);
    blp::paintAdjustArrows(g, pill);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    blp::drawText(g, "Tone", 36, 183, 72, 19, juce::Justification::centredLeft);
    blp::drawText(g, "Volume", 148, 183, 82, 19, juce::Justification::centredLeft);
    g.restoreState();
    blp::paintHeader(g, getWidth(), "BLP Driver");
}
