#include "BlpEditorTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

namespace {
using namespace juce;
using State = AudioProcessorValueTreeState;
const StringArray ratios { "4:1", "8:1", "12:1", "20:1", "All" };

State::ParameterLayout parameters()
{
    State::ParameterLayout p;
    auto knob = [&p](const char* id, const char* name, float lo, float hi, float initial, bool db)
    {
        p.add(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name,
            NormalisableRange<float> { lo, hi, 0.1f }, initial,
            AudioParameterFloatAttributes().withLabel(db ? "dB" : "")
                .withStringFromValueFunction([](float v, int) { return String(v, 1); })));
    };
    knob("input", "Input", 0, 20, 10, true);
    knob("output", "Output", -15, 15, 0, true);
    knob("attack", "Attack", 0, 10, 1.2f, false);
    knob("release", "Release", 0, 10, 5, false);
    p.add(std::make_unique<AudioParameterChoice>(ParameterID { "ratio", 1 }, "Ratio", ratios, 0));
    p.add(std::make_unique<AudioParameterBool>(ParameterID { "bypass", 1 }, "Bypass", false));
    return p;
}

constexpr std::array ids { "input", "output", "attack", "release", "ratio", "bypass" };
struct Preset { const char* name; std::array<float, 6> values; };
constexpr Preset presets[] {
    { "Default", { 10, 0, 1.2f, 5, 0, 0 } },
    { "Vocal Control", { 8, 1, 2, 4, 0, 0 } },
    { "Rap Forward", { 13, 2, 4, 7, 1, 0 } },
    { "Peak Tamer", { 7, 0, 9, 5, 3, 0 } },
    { "All In", { 15, 4, 6, 8, 4, 0 } }
};

class Processor final : public AudioProcessor
{
public:
    Processor() : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true)
                                    .withOutput("Output", AudioChannelSet::stereo(), true)),
                  state(*this, nullptr, "BLP1176", parameters())
    {
        for (size_t i = 0; i < ids.size(); ++i)
            raw[i] = state.getRawParameterValue(ids[i]);
    }

    const String getName() const override { return "BLP BL1176"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        const auto out = layout.getMainOutputChannelSet();
        return out == layout.getMainInputChannelSet()
            && (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo());
    }

    int getNumPrograms() override { return static_cast<int>(std::size(presets)); }
    int getCurrentProgram() override { return program.load(); }
    const String getProgramName(int i) override { return isPositiveAndBelow(i, getNumPrograms()) ? presets[i].name : ""; }
    void changeProgramName(int, const String&) override {}
    void setCurrentProgram(int i) override
    {
        if (!isPositiveAndBelow(i, getNumPrograms()))
            return;
        program.store(i);
        for (size_t n = 0; n < ids.size(); ++n)
        {
            auto* param = state.getParameter(ids[n]);
            param->setValueNotifyingHost(param->convertTo0to1(presets[i].values[n]));
        }
    }

    AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    void getStateInformation(MemoryBlock& data) override
    {
        auto snapshot = state.copyState();
        snapshot.setProperty("program", program.load(), nullptr);
        if (auto xml = snapshot.createXml())
            copyXmlToBinary(*xml, data);
    }
    void setStateInformation(const void* data, int size) override
    {
        auto xml = getXmlFromBinary(data, size);
        if (xml && xml->hasTagName(state.state.getType()))
        {
            auto snapshot = ValueTree::fromXml(*xml);
            program.store(jlimit(0, getNumPrograms() - 1, static_cast<int>(snapshot.getProperty("program", 0))));
            state.replaceState(snapshot);
        }
    }

    void prepareToPlay(double rate, int) override
    {
        sampleRate = rate;
        reduction = 0;
        const auto values = targets(false);
        for (size_t i = 0; i < smooth.size(); ++i)
        {
            smooth[i].reset(rate, 0.005);
            smooth[i].setCurrentAndTargetValue(values[i]);
        }
    }
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& audio, MidiBuffer&) override { process(audio, false); }
    void processBlockBypassed(AudioBuffer<float>& audio, MidiBuffer&) override { process(audio, true); }
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    State state;

private:
    std::array<float, 7> targets(bool bypass) const
    {
        constexpr float slopes[] { 0.75f, 0.875f, 11.0f / 12.0f, 0.95f, 0.98f };
        const auto ratio = jlimit(0, 4, roundToInt(raw[4]->load()));
        const auto attackSeconds = 0.0008 * std::pow(0.025, raw[2]->load() / 10.0);
        const auto releaseSeconds = (ratio == 4 ? 0.65 : 1.0)
            * 1.1 * std::pow(0.05 / 1.1, raw[3]->load() / 10.0);
        return { Decibels::decibelsToGain(raw[0]->load()),
                 Decibels::decibelsToGain(raw[1]->load()),
                 static_cast<float>(-std::expm1(-1.0 / (sampleRate * attackSeconds))),
                 static_cast<float>(-std::expm1(-1.0 / (sampleRate * releaseSeconds))),
                 slopes[ratio], ratio == 4 ? -21.0f : -18.0f,
                 bypass || raw[5]->load() >= 0.5f ? 0.0f : 1.0f };
    }

    void process(AudioBuffer<float>& audio, bool bypass)
    {
        ScopedNoDenormals noDenormals;
        const auto values = targets(bypass);
        for (size_t i = 0; i < smooth.size(); ++i)
            smooth[i].setTargetValue(values[i]);

        auto channels = audio.getArrayOfWritePointers();
        const auto count = audio.getNumChannels();
        for (int i = 0; i < audio.getNumSamples(); ++i)
        {
            const auto input = smooth[0].getNextValue();
            const auto output = smooth[1].getNextValue();
            const auto attack = smooth[2].getNextValue();
            const auto release = smooth[3].getNextValue();
            const auto slope = smooth[4].getNextValue();
            const auto threshold = smooth[5].getNextValue();
            const auto mix = smooth[6].getNextValue();

            float peak = 0;
            for (int c = 0; c < count; ++c)
                peak = jmax(peak, std::abs(channels[c][i]));

            const auto over = Decibels::gainToDecibels(peak * input, -120.0f) - threshold;
            // A 4 dB soft knee blends the slopes around the threshold.
            const auto knee = jlimit(0.0f, 4.0f, over + 2.0f);
            const auto target = slope * (knee * knee / 8.0f + jmax(0.0f, over - 2.0f));
            reduction += (target - reduction) * (target > reduction ? attack : release);
            const auto wetGain = input * output * Decibels::decibelsToGain(-reduction, -160.0f);
            const auto gain = 1.0f + mix * (wetGain - 1.0f);
            for (int c = 0; c < count; ++c)
                channels[c][i] *= gain;
        }
    }

    std::array<std::atomic<float>*, 6> raw {};
    std::array<SmoothedValue<float>, 7> smooth;
    std::atomic<int> program { 0 };
    double sampleRate = 44100;
    float reduction = 0;
};

class KnobStyle final : public blp::EditorLookAndFeel
{
public:
    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float position,
                          float start, float end, Slider&) override
    {
        const auto radius = 23.0f;
        const auto centre = Rectangle<int>(x, y, w, h).toFloat().getCentre();
        const auto angle = jmap(position, start, end);
        auto arc = [&](float finish, Colour colour)
        {
            if (finish <= start)
                return;
            Path p;
            p.addCentredArc(centre.x, centre.y, radius + 5, radius + 5, 0, start, finish, true);
            g.setColour(colour);
            g.strokePath(p, PathStrokeType(2.5f, PathStrokeType::curved, PathStrokeType::rounded));
        };
        arc(end, Colour(0xff47474f));
        arc(angle, Colour(0xffee1c25));
        g.setColour(Colour(0xfff6f5f7));
        g.fillEllipse(Rectangle<float>(radius * 2, radius * 2).withCentre(centre));
        Path mark;
        mark.addRoundedRectangle(-1.4f, -radius + 5, 2.8f, 11, 1.3f);
        g.setColour(Colour(0xff323238));
        g.fillPath(mark, AffineTransform::rotation(angle).translated(centre.x, centre.y));
    }
};

class Editor final : public blp::ThemedEditor
{
public:
    explicit Editor(Processor& p) : blp::ThemedEditor(p)
    {
        setLookAndFeel(&style);
        constexpr std::array names { "Input", "Output", "Attack", "Release" };
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            auto& knob = knobs[i];
            knob.setName(names[i]);
            knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(Slider::TextBoxBelow, false, 70, 19);
            knob.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            knob.setColour(Slider::textBoxTextColourId, blp::textWhite);
            addAndMakeVisible(knob);
            blp::showValueOnHover(knob, *this);
            attachments[i] = std::make_unique<State::SliderAttachment>(p.state, ids[i], knob);
            labels[i].setText(names[i], dontSendNotification);
            labels[i].setJustificationType(Justification::centred);
            labels[i].setColour(Label::textColourId, blp::textWhite);
            addAndMakeVisible(labels[i]);
        }
        ratio.addItemList(ratios, 1);
        ratio.setName("Ratio");
        addAndMakeVisible(ratio);
        addAndMakeVisible(bypass);
        ratioAttachment = std::make_unique<State::ComboBoxAttachment>(p.state, "ratio", ratio);
        bypassAttachment = std::make_unique<State::ButtonAttachment>(p.state, "bypass", bypass);
        setSize(430, 270);
    }

    ~Editor() override { setLookAndFeel(nullptr); }
    void paint(Graphics& g) override
    {
        g.fillAll(blp::canvas);
        g.setColour(Colour(0xfff6f5f7));
        g.setFont(FontOptions(18.0f));
        
        g.setColour(Colour(0xffee1c25));
        g.setFont(FontOptions(12.0f));
        blp::paintHeader(g, getWidth(), "BLP BL1176");
}

    void resized() override
    {
        bypass.setBounds(blp::bypassBounds(getWidth()));
        for (size_t i = 0; i < knobs.size(); ++i)
        {
            const auto x = 21 + 100 * static_cast<int>(i);
            knobs[i].setBounds(x, 65, 88, 87);
            labels[i].setBounds(x, 135, 88, 19);
        }
        ratio.setBounds(127, 210, 176, 34);
    }

private:
    KnobStyle style;
    std::array<Slider, 4> knobs;
    std::array<Label, 4> labels;
    blp::MenuBox ratio { "Ratio" };
    blp::BypassButton bypass;
    std::array<std::unique_ptr<State::SliderAttachment>, 4> attachments;
    std::unique_ptr<State::ComboBoxAttachment> ratioAttachment;
    std::unique_ptr<State::ButtonAttachment> bypassAttachment;
};

AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
} // namespace

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor; }
