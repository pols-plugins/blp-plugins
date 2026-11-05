#include "BlpEditorTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

namespace
{
using namespace juce;
using State = AudioProcessorValueTreeState;
const StringArray notes { "1/2", "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T" };
constexpr std::array<double, 9> beats { 2., 1., 2./3., .75, .5, 1./3., .375, .25, 1./6. };
constexpr double tau = MathConstants<double>::twoPi;

State::ParameterLayout parameters()
{
    State::ParameterLayout result;
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"mode", 1}, "Rate Mode", StringArray{"beat", "Hz"}, 0),
               std::make_unique<AudioParameterChoice>(ParameterID{"division", 1}, "Beat Rate", notes, 1),
               std::make_unique<AudioParameterFloat>(ParameterID{"hz", 1}, "Free Rate",
                   NormalisableRange<float>{0.10f, 11.f, .01f, .55f}, .5f,
                   AudioParameterFloatAttributes().withLabel("Hz").withStringFromValueFunction(
                       [](float value, int) { return String(value, 2); })),
               std::make_unique<AudioParameterFloat>(ParameterID{"spread", 1}, "Spread",
                   NormalisableRange<float>{0.f, 100.f, 1.f}, 70.f, AudioParameterFloatAttributes().withLabel("%")),
               std::make_unique<AudioParameterBool>(ParameterID{"bypass", 1}, "Bypass", false));
    return result;
}

class Processor final : public AudioProcessor
{
public:
    Processor() : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true)
                                                   .withOutput("Output", AudioChannelSet::stereo(), true)) {}
    State state {*this, nullptr, "BLPAutoPan", parameters()};
    const String getName() const override { return "BLP Auto Pan"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        const auto input = layout.getMainInputChannelSet();
        return layout.getMainOutputChannelSet() == AudioChannelSet::stereo()
            && (input == AudioChannelSet::stereo() || input == AudioChannelSet::mono());
    }
    void prepareToPlay(double sampleRate, int) override
    {
        rate = sampleRate;
        phase = 0;
        freeRate.reset(rate, .03);
        freeRate.setCurrentAndTargetValue(hz->load());
        amount.reset(rate, .015);
        amount.setCurrentAndTargetValue(bypass->load() >= .5f ? 0.f : spread->load() * .01f);
        correctionDecay = std::exp(-1. / (.002 * rate));
        phaseCorrection = 0.;
        wasLocked = false;
    }
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override { render(buffer, false); }
    void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer&) override { render(buffer, true); }
    void getStateInformation(MemoryBlock& data) override
    {
        if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, data);
    }
    void setStateInformation(const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType()))
            state.replaceState(ValueTree::fromXml(*xml));
    }

private:
    void render(AudioBuffer<float>& buffer, bool hostBypassed)
    {
        ScopedNoDenormals noDenormals;
        const bool mono = getTotalNumInputChannels() == 1;
        const bool synced = mode->load() < .5f;
        const double duration = beats[static_cast<size_t>(division->load())];
        double bpm = 120.;
        bool locked = false;
        if (synced)
        {
            if (auto* playhead = getPlayHead())
                if (auto position = playhead->getPosition())
                {
                    if (auto value = position->getBpm(); value && std::isfinite(*value) && *value > 0.)
                        bpm = *value;
                    if (auto ppq = position->getPpqPosition(); position->getIsPlaying() && ppq && std::isfinite(*ppq))
                    {
                        const double timelinePhase = wrap(*ppq / duration);
                        // Preserve continuity at seeks and loops, then converge to the timeline.
                        phaseCorrection = std::remainder(phase + phaseCorrection - timelinePhase, 1.);
                        phase = timelinePhase;
                        locked = true;
                    }
                }
        }
        // While stopped, audition with a running oscillator. On playback, follow PPQ including loop jumps.
        const double increment = bpm / (60. * duration * rate);

        if (synced && !locked && wasLocked)
            freeRate.setCurrentAndTargetValue(bpm / (60. * duration));
        freeRate.setTargetValue(synced ? bpm / (60. * duration) : hz->load());
        if (locked)
            freeRate.setCurrentAndTargetValue(freeRate.getTargetValue());
        wasLocked = locked;
        amount.setTargetValue(hostBypassed || bypass->load() >= .5f ? 0.f : spread->load() * .01f);
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float p = static_cast<float>(std::sin(tau * (phase + phaseCorrection))) * amount.getNextValue();
            phaseCorrection *= correctionDecay;
            if (mono)
            {
                constexpr float centre = MathConstants<float>::halfPi * .5f;
                const float angle = (p + 1.f) * centre;
                const float input = left[i];
                left[i] = input * std::cos(angle);
                right[i] = input * std::sin(angle);
            }
            else if (p < 0.f)
                right[i] *= std::cos(p * MathConstants<float>::halfPi);
            else if (p > 0.f)
                left[i] *= std::cos(p * MathConstants<float>::halfPi);
            const double frequency = freeRate.getNextValue();
            phase = wrap(phase + (locked ? increment : frequency / rate));
        }
    }
    static double wrap(double cycles) { return cycles - std::floor(cycles); }

    std::atomic<float>* mode = state.getRawParameterValue("mode");
    std::atomic<float>* division = state.getRawParameterValue("division");
    std::atomic<float>* hz = state.getRawParameterValue("hz");
    std::atomic<float>* spread = state.getRawParameterValue("spread");
    std::atomic<float>* bypass = state.getRawParameterValue("bypass");
    SmoothedValue<double> freeRate;
    SmoothedValue<float> amount;
    double rate = 44100., phase = 0.;
    double phaseCorrection = 0., correctionDecay = 0.;
    bool wasLocked = false;
};

class KnobLook final : public blp::EditorLookAndFeel
{
public:
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float proportion,
                          float start, float end, Slider&) override
    {
        const float radius = static_cast<float>(jmin(width, height)) * .43f;
        const auto centre = Rectangle<int>(x, y, width, height).toFloat().getCentre();
        const float angle = start + proportion * (end - start);
        Path track, progress;
        track.addCentredArc(centre.x, centre.y, radius, radius, 0.f, start, end, true);
        progress.addCentredArc(centre.x, centre.y, radius, radius, 0.f, start, angle, true);
        const PathStrokeType stroke(2.f, PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour(Colour(0xff323238));
        g.strokePath(track, stroke);
        g.setColour(Colour(0xffee1c25));
        g.strokePath(progress, stroke);
        const float face = radius * .71f;
        g.setColour(Colour(0xfff6f5f7));
        g.fillEllipse(centre.x - face, centre.y - face, face * 2, face * 2);
        const Point<float> direction(std::sin(angle), -std::cos(angle));
        g.setColour(Colour(0xff222226));
        g.drawLine(Line<float>(centre + direction * (face * .22f), centre + direction * (face * .76f)), 2.f);
    }
};

class Editor final : public blp::ThemedEditor, private Timer
{
    using SliderBinding = State::SliderAttachment;
public:
    explicit Editor(Processor& processor) : blp::ThemedEditor(processor), state(processor.state)
    {
        configure(beatKnob, "Beat Rate");
        configure(hzKnob, "Free Rate");
        configure(spreadKnob, "Spread");
        spreadKnob.setBounds(62, 138, 76, 76);
        beatKnob.setBounds(73, 49, 54, 54);
        hzKnob.setBounds(beatKnob.getBounds());
        addAndMakeVisible(modeButton);
        addAndMakeVisible(bypassButton);
        modeButton.setColour(TextButton::buttonColourId, Colour(0xff323238));
        modeButton.setColour(TextButton::buttonOnColourId, Colour(0xff323238));
        modeButton.setBounds(101, 107, 30, 15);
        modeButton.onClick = [this]
        {
            auto* parameter = state.getParameter("mode");
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(1.f - parameter->getValue());
            parameter->endChangeGesture();
            refresh();
        };
        modeButton.setTooltip("Switch between tempo sync and free-running rate");
        
        bypassBinding = std::make_unique<State::ButtonAttachment>(state, "bypass", bypassButton);
        setSize(200, 270);
        bypassButton.setBounds(blp::bypassBounds(getWidth()));
        beatKnob.textFromValueFunction = [](double value) { return notes[roundToInt(value)]; };
        hzKnob.textFromValueFunction = [](double value) { return String(value, 2) + " Hz"; };
        spreadKnob.textFromValueFunction = [](double value) { return String(roundToInt(value)) + "%"; };
        refresh();
        startTimerHz(30);
    }
    ~Editor() override
    {
        stopTimer();
        for (auto* slider : {&beatKnob, &hzKnob, &spreadKnob}) slider->setLookAndFeel(nullptr);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(blp::canvas);
        g.setColour(Colours::white);
        g.setFont(16.f);
        
        g.setColour(blp::accent);
        g.setFont(13.f);
        blp::drawText(g, "Rate", 65, 105, 31, 19, Justification::centredRight);
        blp::drawText(g, "Spread", 0, 217, getWidth(), 19, Justification::centred);
        g.setFont(12.f);
        blp::paintHeader(g, getWidth(), "BLP Auto Pan");
}

private:
    void configure(Slider& slider, const String& label)
    {
        addAndMakeVisible(slider);
        slider.setName(label);
        slider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&look);
        blp::showValueOnHover(slider, *this);
    }
    void timerCallback() override { refresh(); }
    void refresh()
    {
        const bool beat = state.getRawParameterValue("mode")->load() < .5f;
        beatKnob.setVisible(beat);
        hzKnob.setVisible(!beat);
        modeButton.setButtonText(beat ? "beat" : "Hz");
    }

    State& state;
    KnobLook look;
    Slider beatKnob, hzKnob, spreadKnob;
    TextButton modeButton;
    blp::BypassButton bypassButton;
    SliderBinding beatBinding {state, "division", beatKnob};
    SliderBinding hzBinding {state, "hz", hzKnob};
    SliderBinding spreadBinding {state, "spread", spreadKnob};
    std::unique_ptr<State::ButtonAttachment> bypassBinding;
    TooltipWindow tooltip {this, 500};
};

AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor; }


