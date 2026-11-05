#include "BlpEditorTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>
#include <vector>

namespace {
using namespace juce;
constexpr int roomCount = 8;
constexpr const char* names[] {
    "Atrium", "Classroom", "Campus Steps", "Rhythm Chamber", "Innovation Hall",
    "Dorm Hall", "Union Stairwell", "Women's Institute"
};
constexpr const char* descriptions[] {
    "Open, airy reverb with reflective space.",
    "Crisp, focused reverb with short echoes.",
    "Open, outdoor reverb with scattered reflections.",
    "Tight, rhythmic reverb with clear definition.",
    "Spacious, modern reverb with smooth decay.",
    "Narrow, echoey reverb with lively reflections.",
    "Resonant, stairwell reverb with natural echoes.",
    "Warm, gentle reverb with soft reflections."
};

struct Room {
    std::array<float, 4> earlyMs, lateMs;
    float decay, dampingHz, bassHz, diffusion, width, early, late;
};
constexpr Room rooms[] {
    {{19, 34, 51, 74}, {43, 57, 69, 83}, 3.40f, 7600, 100, .72f, .98f, .33f, .66f},
    {{7, 12, 19, 27}, {13, 17, 23, 29}, .44f, 5600, 260, .53f, .53f, .50f, .44f},
    {{28, 63, 116, 173}, {31, 47, 67, 89}, .78f, 6200, 210, .25f, .94f, .64f, .13f},
    {{10, 17, 29, 43}, {19, 29, 37, 43}, .73f, 4500, 155, .58f, .67f, .43f, .55f},
    {{17, 28, 44, 59}, {31, 43, 59, 73}, 2.30f, 5800, 105, .75f, .90f, .33f, .66f},
    {{12, 24, 37, 51}, {23, 37, 47, 61}, 1.18f, 4800, 130, .42f, .28f, .62f, .43f},
    {{24, 49, 77, 109}, {31, 53, 71, 97}, 2.85f, 3400, 85, .38f, .59f, .66f, .52f},
    {{11, 21, 34, 46}, {23, 31, 47, 61}, 1.55f, 3150, 110, .68f, .73f, .30f, .62f}
};

struct Delay {
    std::vector<float> data;
    size_t cursor = 0;
    void prepare(int samples) { data.assign((size_t) jmax(1, samples), 0.f); cursor = 0; }
    float read() const { return data[cursor]; }
    void push(float x) { data[cursor] = x; if (++cursor == data.size()) cursor = 0; }
    float diffuse(float x, float gain)
    {
        const auto y = read() - gain * x;
        push(x + gain * y);
        return y;
    }
};

struct Engine {
    std::array<Delay, 4> earlyL, earlyR, late;
    std::array<Delay, 2> diffusionL, diffusionR;
    std::array<float, 4> feedback {}, damp {};
    float lowL = 0, lowR = 0, bass = 0, damping = 0;
    const Room* room = nullptr;

    void prepare(const Room& config, double rate)
    {
        room = &config;
        const auto samples = [rate](float ms) { return roundToInt(ms * rate * .001); };
        const auto coefficient = [rate](float hz) {
            return (float) (1.0 - std::exp(-MathConstants<double>::twoPi * hz / rate));
        };
        for (size_t i = 0; i < late.size(); ++i)
        {
            earlyL[i].prepare(samples(config.earlyMs[i]));
            earlyR[i].prepare(samples(config.earlyMs[i] * (i % 2 == 0 ? 1.11f : .91f)));
            late[i].prepare(samples(config.lateMs[i]));
            feedback[i] = std::pow(0.001f, config.lateMs[i] * .001f / config.decay);
        }
        diffusionL[0].prepare(samples(2.8f + config.lateMs[0] * .08f));
        diffusionL[1].prepare(samples(4.1f + config.lateMs[1] * .10f));
        diffusionR[0].prepare(samples(3.4f + config.lateMs[0] * .08f));
        diffusionR[1].prepare(samples(5.3f + config.lateMs[1] * .10f));
        bass = coefficient(config.bassHz);
        damping = coefficient(config.dampingHz);
        damp.fill(0);
        lowL = lowR = 0;
    }

    std::array<float, 2> process(float l, float r)
    {
        lowL += bass * (l - lowL);
        lowR += bass * (r - lowR);
        l -= .75f * lowL;
        r -= .75f * lowR;
        float reflectionsL = 0, reflectionsR = 0, sum = 0;
        std::array<float, 4> taps {};
        constexpr float gains[] { .49f, .31f, -.24f, .17f };
        for (size_t i = 0; i < late.size(); ++i)
        {
            reflectionsL += gains[i] * earlyL[i].read();
            reflectionsR += gains[i] * earlyR[i].read();
            earlyL[i].push(.82f * l + .18f * r);
            earlyR[i].push(.82f * r + .18f * l);
            taps[i] = late[i].read();
            damp[i] += damping * (taps[i] - damp[i]);
            sum += damp[i];
        }
        auto inputL = .65f * l + .35f * reflectionsL;
        auto inputR = .65f * r + .35f * reflectionsR;
        for (size_t i = 0; i < diffusionL.size(); ++i)
        {
            inputL = diffusionL[i].diffuse(inputL, room->diffusion);
            inputR = diffusionR[i].diffuse(inputR, room->diffusion);
        }
        const float injection[] {inputL, inputR, -inputL, -inputR};
        for (size_t i = 0; i < late.size(); ++i)
            late[i].push(.37f * injection[i] + feedback[i] * (.5f * sum - damp[i]));

        const auto wetL = room->early * reflectionsL + room->late * .5f * (taps[0] + taps[1] - taps[2] + taps[3]);
        const auto wetR = room->early * reflectionsR + room->late * .5f * (taps[0] - taps[1] - taps[2] - taps[3]);
        const auto mid = .5f * (wetL + wetR);
        const auto side = .5f * room->width * (wetL - wetR);
        return {mid + side, mid - side};
    }
};

class Processor final : public AudioProcessor {
public:
    Processor() : AudioProcessor(BusesProperties()
        .withInput("Input", AudioChannelSet::stereo(), true)
        .withOutput("Output", AudioChannelSet::stereo(), true)),
        state(*this, nullptr, "Univerb", layout()) {}

    AudioProcessorValueTreeState state;
    const String getName() const override { return "BLP Univerb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    bool isBusesLayoutSupported(const BusesLayout& buses) const override
    {
        const auto out = buses.getMainOutputChannelSet();
        return buses.getMainInputChannelSet() == out
            && (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo());
    }
    void prepareToPlay(double rate, int) override
    {
        for (int i = 0; i < roomCount; ++i)
        {
            engines[(size_t) i].prepare(rooms[i], rate);
            weights[(size_t) i].reset(rate, .080);
            weights[(size_t) i].setCurrentAndTargetValue(i == selected() ? 1.f : 0.f);
        }
        mix.reset(rate, .025);
        active.reset(rate, .025);
        mix.setCurrentAndTargetValue(mixValue->load() * .01f);
        active.setCurrentAndTargetValue(bypassValue->load() < .5f ? 1.f : 0.f);
    }
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override { process(buffer, false); }
    void processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer&) override { process(buffer, true); }
    void getStateInformation(MemoryBlock& data) override
    {
        if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, data);
    }
    void setStateInformation(const void* data, int bytes) override
    {
        if (auto xml = getXmlFromBinary(data, bytes))
            if (xml->hasTagName(state.state.getType())) state.replaceState(ValueTree::fromXml(*xml));
    }

private:
    static AudioProcessorValueTreeState::ParameterLayout layout()
    {
        StringArray choices;
        for (auto name : names) choices.add(name);
        AudioProcessorValueTreeState::ParameterLayout result;
        result.add(std::make_unique<AudioParameterFloat>(ParameterID{"mix", 1}, "Mix",
            NormalisableRange<float>{0.f, 100.f}, 35.f,
            AudioParameterFloatAttributes().withLabel("%")));
        result.add(std::make_unique<AudioParameterChoice>(ParameterID{"type", 1}, "Type", choices, 7));
        result.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass", 1}, "Bypass", false));
        return result;
    }
    int selected() const { return jlimit(0, roomCount - 1, roundToInt(typeValue->load())); }
    void process(AudioBuffer<float>& buffer, bool hostBypassed)
    {
        ScopedNoDenormals noDenormals;
        if (buffer.getNumChannels() == 0) return;
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
        const auto target = selected();
        for (int i = 0; i < roomCount; ++i)
            weights[(size_t) i].setTargetValue(i == target ? 1.f : 0.f);
        mix.setTargetValue(mixValue->load() * .01f);
        active.setTargetValue(!hostBypassed && bypassValue->load() < .5f ? 1.f : 0.f);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const auto l = left[n];
            const auto r = right ? right[n] : l;
            float wetL = 0, wetR = 0;
            // Keep all rooms fed so any preset can fade in with a continuous tail.
            for (size_t i = 0; i < engines.size(); ++i)
            {
                const auto wet = engines[i].process(l, r);
                const auto weight = weights[i].getNextValue();
                wetL += weight * wet[0];
                wetR += weight * wet[1];
            }
            const auto blend = mix.getNextValue() * active.getNextValue();
            if (blend == 0.f) continue;
            left[n] = (1.f - blend) * l + blend * (right ? wetL : .5f * (wetL + wetR));
            if (right) right[n] = (1.f - blend) * r + blend * wetR;
        }
    }
    std::atomic<float>* mixValue = state.getRawParameterValue("mix");
    std::atomic<float>* typeValue = state.getRawParameterValue("type");
    std::atomic<float>* bypassValue = state.getRawParameterValue("bypass");
    std::array<Engine, roomCount> engines;
    std::array<SmoothedValue<float>, roomCount> weights;
    SmoothedValue<float> mix, active;
};

const Colour cyan { 0xffee1c25 };
const Colour panel { 0xff151518 };
const Colour field { 0xff222226 };
const Colour muted { 0xffb9b8c0 };

class Skin final : public blp::MenuLookAndFeel {
public:
    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float position,
                          float start, float end, Slider&) override
    {
        const Point<float> centre((float) x + (float) w * .5f, (float) y + (float) h * .5f);
        const auto angle = start + position * (end - start);
        Path arc;
        arc.addCentredArc(centre.x, centre.y, 40.f, 40.f, 0.f, start, end, true);
        g.setColour(field.brighter(.10f));
        const PathStrokeType stroke(1.75f, PathStrokeType::curved, PathStrokeType::rounded);
        g.strokePath(arc, stroke);
        if (position > 0.f)
        {
            Path fill;
            fill.addCentredArc(centre.x, centre.y, 40.f, 40.f, 0.f, start, angle, true);
            g.setColour(cyan);
            g.strokePath(fill, stroke);
        }
        g.setColour(Colours::white);
        g.fillEllipse(Rectangle<float>(63, 63).withCentre(centre));
        const auto direction = Point<float>(std::sin(angle), -std::cos(angle));
        g.setColour(Colour(0xff47474f));
        g.drawLine({centre + direction * 7.f, centre + direction * 25.f}, 2.4f);
    }
};

class RoomRow final : public PopupMenu::CustomComponent {
public:
    RoomRow(int index, bool current) : CustomComponent(true), room(index), selected(current) {}
    void getIdealSize(int& w, int& h) override { w = 310; h = 42; }
    void paint(Graphics& g) override
    {
        const bool highlighted = isItemHighlighted();
        if (highlighted) g.fillAll(Colour(0xff47474f));
        if (selected)
        {
            g.setColour(cyan);
            g.fillEllipse(8.0f, highlighted ? 9.0f : 18.0f, 5.0f, 5.0f);
        }
        g.setColour(Colours::white);
        g.setFont(14.f);
        const auto textArea = getLocalBounds().withTrimmedLeft(20).withTrimmedRight(7);
        blp::drawText(g, names[room], highlighted ? textArea.withHeight(23) : textArea, Justification::centredLeft);
        if (highlighted)
        {
            g.setFont(12.f);
            blp::drawFittedText(g, descriptions[room], textArea.withTrimmedTop(22),
                               Justification::centredLeft, 1);
        }
    }
private:
    int room;
    bool selected;
};

class TypeButton final : public Button {
public:
    TypeButton() : Button("Type") {}
    String selection { names[7] };
    void paintButton(Graphics& g, bool, bool) override
    {
        blp::paintMenuField(g, getWidth(), getHeight(), "Type");
        const auto left = blp::menuValueStart("Type");
        g.setFont(FontOptions(14.f));
        blp::drawFittedText(g, selection, left, 0, getWidth() - left - 26,
                           getHeight(), Justification::centredLeft, 1);
    }
};

class Editor final : public blp::ThemedEditor {
public:
    explicit Editor(Processor& p)
        : blp::ThemedEditor(p), mixAttachment(p.state, "mix", knob),
          bypassAttachment(p.state, "bypass", bypass),
          typeAttachment(*p.state.getParameter("type"), [this](float value) {
              room = jlimit(0, roomCount - 1, roundToInt(value));
              type.selection = names[room];
              type.setTitle("Type, " + type.selection);
              type.repaint();
          }, nullptr)
    {
        knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        knob.setRotaryParameters(MathConstants<float>::pi * 1.33f,
                                 MathConstants<float>::pi * 2.67f, true);
        knob.setDoubleClickReturnValue(true, 35.);
        knob.setTitle("Mix");
        knob.setLookAndFeel(&skin);
        bypass.onStateChange = [this] { refreshBypass(); };
        type.onClick = [this] {
            PopupMenu menu;
            menu.setLookAndFeel(&skin);
            for (int i = 0; i < roomCount; ++i)
                menu.addCustomItem(i + 1, std::make_unique<RoomRow>(i, i == room), nullptr, names[i]);
            const Component::SafePointer<Editor> safe(this);
            menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&type)
                .withMaximumNumColumns(1).withItemThatMustBeVisible(room + 1),
                [safe](int result) {
                    if (safe && result > 0) safe->typeAttachment.setValueAsCompleteGesture((float) (result - 1));
                });
        };
        for (Component* component : std::array<Component*, 3>{ &knob, &bypass, &type })
            addAndMakeVisible(component);
        typeAttachment.sendInitialUpdate();
        refreshBypass();
        setSize(214, 270);
    }
    ~Editor() override
    {
        PopupMenu::dismissAllActiveMenus();
        knob.setLookAndFeel(nullptr);
    }
    void resized() override
    {
        bypass.setBounds(blp::bypassBounds(getWidth()));
        knob.setBounds(55, 60, 96, 96);
        type.setBounds(12, 214, 190, 34);
    }
    void paint(Graphics& g) override
    {
        g.fillAll(blp::canvas);
        g.setColour(field);
        g.drawRect(getLocalBounds());
        g.setColour(Colours::white);
        g.setFont(Font(FontOptions(17.f)));
        
        g.setColour(muted);
        g.setFont(14.f);
        blp::drawText(g, "Mix", 65, 159, 76, 17, Justification::centred);
        blp::paintHeader(g, getWidth(), "BLP Univerb");
}
private:
    void refreshBypass()
    {
        const auto enabled = !bypass.getToggleState();
        knob.setEnabled(enabled);
        type.setEnabled(enabled);
        repaint();
    }
    Skin skin;
    Slider knob;
    blp::BypassButton bypass;
    TypeButton type;
    AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
    ParameterAttachment typeAttachment;
    int room = 7;
};

AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor(); }

