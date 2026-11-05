#include "BlpEditorTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "SpringTank.h"

namespace
{
using namespace juce;

class Processor final : public AudioProcessor
{
public:
    Processor()
        : AudioProcessor(BusesProperties()
              .withInput("Input", AudioChannelSet::stereo(), true)
              .withOutput("Output", AudioChannelSet::stereo(), true)),
          state(*this, nullptr, "SpringReverb", makeParameters()),
          amount(*state.getRawParameterValue("amount")),
          type(*state.getRawParameterValue("type")),
          bypass(*state.getRawParameterValue("bypass"))
    {
    }

    AudioProcessorValueTreeState state;

    const String getName() const override { return "BLP Spring Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;

    AudioProcessorParameter* getBypassParameter() const override
    {
        return state.getParameter("bypass");
    }

    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        const auto output = layout.getMainOutputChannelSet();
        return output == layout.getMainInputChannelSet()
            && (output == AudioChannelSet::mono() || output == AudioChannelSet::stereo());
    }

    void prepareToPlay(double sampleRate, int) override
    {
        wet.reset(sampleRate, 0.035);
        mode.reset(sampleRate, 0.080);
        for (int channel = 0; channel < getTotalNumOutputChannels(); ++channel)
        {
            american[channel].prepare(static_cast<float>(sampleRate), false, channel);
            uk[channel].prepare(static_cast<float>(sampleRate), true, channel);
        }
        reset();
    }

    void reset() override
    {
        for (auto& tank : american) tank.clear();
        for (auto& tank : uk) tank.clear();

        wet.setCurrentAndTargetValue(bypass.load() < 0.5f ? amount.load() * 0.01f : 0.0f);
        mode.setCurrentAndTargetValue(type.load());
    }

    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& audio, MidiBuffer&) override { render(audio, false); }
    void processBlockBypassed(AudioBuffer<float>& audio, MidiBuffer&) override { render(audio, true); }

    void getStateInformation(MemoryBlock& dest) override
    {
        if (const auto xml = state.copyState().createXml())
            copyXmlToBinary(*xml, dest);
    }

    void setStateInformation(const void* data, int bytes) override
    {
        if (const auto xml = getXmlFromBinary(data, bytes))
            if (xml->hasTagName(state.state.getType()))
                state.replaceState(ValueTree::fromXml(*xml));
    }

private:
    static AudioProcessorValueTreeState::ParameterLayout makeParameters()
    {
        AudioProcessorValueTreeState::ParameterLayout params;
        params.add(std::make_unique<AudioParameterFloat>(
            ParameterID { "amount", 1 }, "Amount", NormalisableRange<float> { 0, 100, 0.1f }, 20.0f,
            AudioParameterFloatAttributes()
                .withLabel("%")
                .withStringFromValueFunction([] (float x, int) { return String(x, 1); })
                .withValueFromStringFunction([] (const String& x) { return x.getFloatValue(); })));
        params.add(std::make_unique<AudioParameterChoice>(
            ParameterID { "type", 1 }, "Type", StringArray { "American", "UK" }, 0));
        params.add(std::make_unique<AudioParameterBool>(ParameterID { "bypass", 1 }, "Bypass", false));
        return params;
    }

    void render(AudioBuffer<float>& audio, bool hostBypassed) noexcept
    {
        ScopedNoDenormals noDenormals;
        const auto channels = jmin(audio.getNumChannels(), getTotalNumOutputChannels(), 2);
        wet.setTargetValue(hostBypassed || bypass.load() >= 0.5f ? 0.0f : amount.load() * 0.01f);
        mode.setTargetValue(type.load());
        auto* const* samples = audio.getArrayOfWritePointers();

        for (int i = 0; i < audio.getNumSamples(); ++i)
        {
            const float mix = wet.getNextValue();
            const float voicing = mode.getNextValue();
            // Keep both tanks excited so switching types blends their existing tails.
            for (int c = 0; c < channels; ++c)
            {
                const float dry = samples[c][i];
                const float a = american[c].tick(dry);
                const float b = uk[c].tick(dry);
                if (mix > 0)
                {
                    const float reverb = a + voicing * (b - a);
                    samples[c][i] = dry * (1 - 0.22f * mix) + (1.40f * mix) * reverb;
                }
            }
        }
    }

    std::atomic<float>& amount;
    std::atomic<float>& type;
    std::atomic<float>& bypass;
    std::array<spring::Tank, 2> american, uk;
    SmoothedValue<float> wet, mode;
};

const Colour ink { 0xfff6f5f7 };
const Colour violet { 0xffee1c25 };
const Colour panel { 0xff151518 };

class Skin final : public blp::EditorLookAndFeel
{
public:
    Skin()
    {
        setColour(ComboBox::backgroundColourId, panel);
        setColour(ComboBox::outlineColourId, Colours::transparentBlack);
        setColour(ComboBox::textColourId, blp::textWhite);
        setColour(ComboBox::arrowColourId, ink);
        setColour(PopupMenu::backgroundColourId, Colour(0xff323238));
        setColour(PopupMenu::textColourId, blp::textWhite);
        setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xffee1c25));
        setColour(PopupMenu::highlightedTextColourId, blp::textWhite);
    }

    Slider::SliderLayout getSliderLayout(Slider& slider) override
    {
        return { { 0, 21, slider.getWidth(), slider.getHeight() - 42 }, {} };
    }

    void drawLinearSlider(Graphics& g, int, int, int w, int, float position,
                          float, float, Slider::SliderStyle, Slider& slider) override
    {
        const auto y = roundToInt(position);
        const Rectangle<float> pill { 64, position - 16, 96, 32 };
        g.setColour(ink);
        g.drawHorizontalLine(y, 0, static_cast<float>(w));
        g.setFont(FontOptions(10.0f));
        blp::drawText(g, String(slider.getValue(), 1) + " %", 2, y - 18, 59, 16,
                   Justification::centredLeft);

        g.setColour(Colour(0xff222226));
        g.fillRoundedRectangle(pill, 16);
        g.setColour(slider.hasKeyboardFocus(false) ? Colour(0xffee1c25) : ink);
        g.drawRoundedRectangle(pill, 16, 1);
        g.setFont(FontOptions(13.0f));
        blp::drawText(g, "Amount", 80, y - 10, 54, 20, Justification::centredLeft);
        Path arrows;
        arrows.startNewSubPath(140, position - 1.5f);
        arrows.lineTo(143.5f, position - 5);
        arrows.lineTo(147, position - 1.5f);
        arrows.startNewSubPath(140, position + 1.5f);
        arrows.lineTo(143.5f, position + 5);
        arrows.lineTo(147, position + 1.5f);
        g.strokePath(arrows, PathStrokeType(0.8f));
    }

    void drawComboBox(Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box) override
    {
        const float y = h * 0.5f;
        Path arrow;
        arrow.startNewSubPath(static_cast<float>(w - 14), y - 2);
        arrow.lineTo(static_cast<float>(w - 10), y + 2);
        arrow.lineTo(static_cast<float>(w - 6), y - 2);
        g.setColour(ink);
        g.strokePath(arrow, PathStrokeType(1.15f));
        if (box.hasKeyboardFocus(false))
            g.drawRoundedRectangle(box.getLocalBounds().toFloat().reduced(0.5f), 3, 1);
    }

    void positionComboBoxText(ComboBox& box, Label& label) override
    {
        label.setBounds(0, 0, box.getWidth() - 19, box.getHeight());
        label.setFont(FontOptions(15.0f));
        label.setBorderSize({});
    }

    void drawToggleButton(Graphics& g, ToggleButton& button, bool hover, bool down) override
    {
        const bool off = button.getToggleState();
        const auto bounds = button.getLocalBounds().toFloat().reduced(0.6f);
        const auto fill = off ? Colour(0xffee1c25) : Colour(0xff323238);
        g.setColour(hover || down ? fill.brighter(0.18f) : fill);
        g.fillRoundedRectangle(bounds, 6);
        g.setColour(button.hasKeyboardFocus(false) ? ink : Colour(0xff514260));
        g.drawRoundedRectangle(bounds, 6, 1);
        g.setColour(off ? Colour(0xffee1c25) : ink);
        g.setFont(FontOptions(12.0f));
        blp::drawText(g, "Bypass", bounds, Justification::centred);
    }
};

class Editor final : public blp::ThemedEditor, private Timer
{
public:
    explicit Editor(Processor& processor)
        : blp::ThemedEditor(processor),
          amountParam(*processor.state.getRawParameterValue("amount"))
    {
        setLookAndFeel(&skin);
        setOpaque(true);
        amount.setSliderStyle(Slider::LinearVertical);
        amount.setSliderSnapsToMousePosition(false);
        amount.setMouseDragSensitivity(100);
        amount.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        amount.setDoubleClickReturnValue(true, 20);
        amount.setTitle("Amount");
        type.addItem("American", 1);
        type.addItem("UK", 2);
        type.setTitle("Type");
        for (auto* c : std::array<Component*, 3> { &amount, &type, &bypass })
        {
            addAndMakeVisible(c);
            c->setWantsKeyboardFocus(true);
        }

        amountLink = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.state, "amount", amount);
        typeLink = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state, "type", type);
        bypassLink = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(processor.state, "bypass", bypass);
        shown = amountParam.load() * 0.01f;
        setSize(236, 270);
        timerCallback();
        startTimerHz(40);
    }

    ~Editor() override
    {
        stopTimer();
        setLookAndFeel(nullptr);
    }

    void resized() override
    {
        bypass.setBounds(blp::bypassBounds(getWidth()));
        amount.setBounds(0, 49, 228, 142);
        type.setBounds(30, 215, 176, 34);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(blp::canvas);
        g.setColour(panel);
        g.fillRoundedRectangle(0, -8, 228, getHeight() + 8, 9);
        g.setColour(ink);
        g.setFont(FontOptions(14.0f));
        

        drawSpring(g);
        blp::paintHeader(g, getWidth(), "BLP Spring Reverb");
}

private:
    void timerCallback() override
    {
        const float target = amountParam.load() * 0.01f;
        shown += 0.21f * (target - shown);
        if (std::abs(target - shown) < 0.0002f) shown = target;
        phase += 0.047f;
        if (phase >= MathConstants<float>::twoPi * 10) phase = 0;
        repaint();
    }

    void drawSpring(Graphics& g) const
    {
        const Point<float> center { 112, 121 };
        const float core = 8 + 28 * shown;
        const float reach = 20 + 54 * shown;
        ColourGradient halo(Colour(0xffee1c25).withAlpha((0.11f + 0.31f * shown)),
                            center, Colours::transparentBlack, center.translated(reach, 0), true);
        g.setGradientFill(halo);
        g.fillEllipse(center.x - reach, center.y - reach, reach * 2, reach * 2);

        for (int ring = 0; ring < 7; ++ring)
        {
            const float fade = jlimit(0.0f, 1.0f, (shown - (0.025f + 0.126f * ring)) * 6.7f);
            if (fade <= 0) continue;
            const float radius = core + 4 + ring * (4.2f + 0.8f * shown);
            Path path;
            for (int i = 0; i < 112; ++i)
            {
                const float angle = MathConstants<float>::twoPi * i / 112.0f;
                const float warp = shown * shown * (2.3f * std::sin(3 * angle + phase + ring * 0.52f)
                    + 1.5f * std::cos(5 * angle - 1.4f * phase + ring * 0.61f));
                const auto point = center.translated((radius + warp) * std::cos(angle),
                                                     (radius + warp) * std::sin(angle));
                if (i == 0) path.startNewSubPath(point);
                else path.lineTo(point);
            }
            path.closeSubPath();
            const float alpha = fade * (0.34f + 0.48f * shown);
            g.setColour(violet.withAlpha(alpha * 0.19f));
            g.strokePath(path, PathStrokeType(5.0f));
            g.setColour(violet.withAlpha(alpha));
            g.strokePath(path, PathStrokeType(1.25f));
        }

        const Rectangle<float> disc { center.x - core, center.y - core, core * 2, core * 2 };
        const float outline = jlimit(0.0f, 1.0f, shown * 5);
        ColourGradient orb(violet.withAlpha((0.52f - 0.40f * outline)),
                           center, violet.withAlpha(0.0f), center.translated(core, 0), true);
        g.setGradientFill(orb);
        g.fillEllipse(disc);
        g.setColour(violet.withAlpha(outline * 0.84f));
        g.drawEllipse(disc, 1.0f);
    }

    Skin skin;
    Slider amount;
    blp::MenuBox type { "Type" };
    blp::BypassButton bypass;
    std::atomic<float>& amountParam;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> amountLink;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> typeLink;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> bypassLink;
    float shown = 0.2f, phase = 0;
};

AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor; }
