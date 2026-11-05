#include "BlpEditorTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>
#include <vector>

namespace {
constexpr auto description = "Algorithmic reverb recreating a studio space.";
constexpr std::array<const char*, 3> ids { "mix", "size", "tone" };
constexpr std::array<const char*, 3> names { "Mix", "Size", "Tone" };
constexpr std::array<float, 3> defaults { 50, 25, 90 };

class Room
{
public:
    void prepare(double rate)
    {
        fs = static_cast<float>(rate);
        lastTone = -1;
        for (auto& channel : history)
            channel.resize(static_cast<size_t>(std::ceil(fs * 0.060f)) + 2);
        for (size_t i = 0; i < lines.size(); ++i)
            lines[i].resize(static_cast<size_t>(juce::jmax(1, juce::roundToInt(fs * delays[i]))));
        clear();
    }

    void clear()
    {
        for (auto& channel : history)
            std::fill(channel.begin(), channel.end(), 0.0f);
        for (auto& line : lines)
            std::fill(line.begin(), line.end(), 0.0f);
        cursor = 0;
        positions.fill(0);
        damped.fill(0);
        filtered.fill(0);
    }

    std::array<float, 2> tick(float left, float right, float size, float tone)
    {
        history[0][cursor] = left;
        history[1][cursor] = right;

        const float spacing = 0.65f + 1.25f * size;
        if (size != lastSize)
        {
            lastSize = size;
            const float decay = 0.26f + 1.32f * size;
            for (size_t i = 0; i < gains.size(); ++i)
                gains[i] = std::exp(-6.907755f * delays[i] / decay);
        }
        if (tone != lastTone)
        {
            lastTone = tone;
            const float cutoff = 750.0f + 17250.0f * tone * tone;
            lowpass = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * cutoff / fs);
        }

        std::array<float, 2> early {};
        for (size_t i = 0; i < taps.size(); ++i)
        {
            const float offset = taps[i] * spacing * fs;
            const float l = read(0, offset);
            const float r = read(1, offset * 1.09f);
            const float cross = i % 2 == 0 ? 0.22f : 0.44f;
            early[0] += weights[i] * ((1.0f - cross) * l + cross * r);
            early[1] += weights[i] * ((1.0f - cross) * r + cross * l);
        }
        cursor = (cursor + 1) % history[0].size();

        std::array<float, 8> feedback {};
        float sum = 0;
        float tailL = 0;
        float tailR = 0;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const float output = lines[i][positions[i]];
            damped[i] += lowpass * (output - damped[i]);
            feedback[i] = damped[i] * gains[i];
            sum += feedback[i];
            tailL += output * leftSigns[i];
            tailR += output * rightSigns[i];
        }
        // Orthogonal Householder feedback mixes all delay lines at each return.
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const float feed = 0.18f * (leftSigns[i] * early[0] + rightSigns[i] * early[1]);
            lines[i][positions[i]] = feed + feedback[i] - 0.25f * sum;
            positions[i] = (positions[i] + 1) % lines[i].size();
        }

        filtered[0] += lowpass * (early[0] + 0.38f * tailL - filtered[0]);
        filtered[1] += lowpass * (early[1] + 0.38f * tailR - filtered[1]);
        return filtered;
    }

private:
    float read(size_t channel, float offset) const
    {
        const auto& ring = history[channel];
        const auto whole = static_cast<size_t>(offset);
        const auto first = (cursor + ring.size() - whole) % ring.size();
        const auto second = (first + ring.size() - 1) % ring.size();
        return ring[first] + (offset - static_cast<float>(whole)) * (ring[second] - ring[first]);
    }

    static constexpr std::array<float, 6> taps { .0047f, .0071f, .0109f, .0149f, .0191f, .0253f };
    static constexpr std::array<float, 6> weights { .36f, -.26f, .23f, .16f, -.13f, .095f };
    static constexpr std::array<float, 8> delays { .0127f, .0163f, .0197f, .0239f, .0281f, .0313f, .0359f, .0419f };
    static constexpr std::array<float, 8> leftSigns { 1, -1, 1, -1, 1, -1, 1, -1 };
    static constexpr std::array<float, 8> rightSigns { 1, 1, -1, -1, -1, -1, 1, 1 };
    std::array<std::vector<float>, 2> history;
    std::array<std::vector<float>, 8> lines;
    std::array<size_t, 8> positions {};
    std::array<float, 8> damped {};
    std::array<float, 2> filtered {};
    size_t cursor = 0;
    float fs = 48000;
    float lastSize = -1;
    float lastTone = -1;
    float lowpass = 0;
    std::array<float, 8> gains {};
};

class StudioLook final : public blp::EditorLookAndFeel
{
    int getSliderThumbRadius(juce::Slider& slider) override
    {
        return slider.getName() == "Mix" ? 35 : 9;
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float, float, juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        const auto orange = juce::Colour(0xffee1c25);
        const auto white = juce::Colour(0xfff6f5f7);
        const auto muted = juce::Colour(0xffb9b8c0);
        const auto percent = juce::String(juce::roundToInt(slider.getValue())) + "%";
        if (slider.getName() == "Mix")
        {
            const float amount = static_cast<float>(slider.getValue() / 100.0);
            const float centre = x + w * 0.5f;
            const auto phase = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.0034);
            const auto pulse = 0.5f + 0.5f * std::sin(phase);
            const auto expansion = 1.0f + 0.28f * amount * pulse;
            if (amount > 0.0f)
            {
                const float halfWidth = (35.0f + 48.0f * amount) * expansion;
                const float halfHeight = (27.0f + 24.0f * amount) * expansion;
                const auto amber = blp::accent;
                juce::Graphics::ScopedSaveState saved(g);
                g.addTransform(juce::AffineTransform::scale(halfWidth, halfHeight).translated(centre, pos));
                juce::ColourGradient glow(amber.withAlpha(amount * (0.70f - 0.13f * pulse)), 0, 0,
                                         amber.withAlpha(0.0f), 1, 0, true);
                glow.addColour(0.38, amber.withAlpha(0.39f * amount));
                glow.addColour(0.72, amber.withAlpha(0.08f * amount));
                g.setGradientFill(glow);
                g.fillEllipse(-1, -1, 2, 2);
            }
            g.setColour(white);
            g.drawVerticalLine(juce::roundToInt(centre), float(y), float(y + h));
            g.drawHorizontalLine(juce::roundToInt(pos), float(x), float(x + w));
            const auto halo = (13.0f + 16.0f * amount) * expansion;
            juce::ColourGradient centreGlow(orange.withAlpha(0.72f * amount), centre, pos,
                                            orange.withAlpha(0.0f), centre + halo, pos, true);
            centreGlow.addColour(0.52, orange.withAlpha(0.47f * amount));
            g.setGradientFill(centreGlow);
            g.fillEllipse(centre - halo, pos - halo, halo * 2.0f, halo * 2.0f);
            const juce::Rectangle<float> pill(centre - 30, pos - 10, 60, 20);
            g.setColour(juce::Colour(0xff222226));
            g.fillRoundedRectangle(pill, 9);
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(pill, 9.0f, 1.0f);
            g.setColour(white);
            g.setFont(13.0f);
            blp::drawText(g, "Mix", pill.toNearestInt().translated(-5, 0), juce::Justification::centred);
            blp::paintAdjustArrows(g, pill);
            blp::drawText(g, percent, x + 6, juce::roundToInt(pos) - 19, 43, 17, juce::Justification::centredLeft);
            return;
        }

        const float track = float(x + w - 15);
        g.setColour(juce::Colour(0xff47474f));
        g.drawVerticalLine(juce::roundToInt(track), float(y), float(y + h));
        g.setColour(orange);
        g.fillRect(track - 1.0f, pos, 2.0f, y + h - pos);
        g.fillRect(track - 8.0f, pos - 1.0f, 16.0f, 2.0f);
        g.setColour(muted);
        g.setFont(12.0f);
        blp::drawText(g, slider.getName(), x + 6, y + 2, w - 30, 16, juce::Justification::centredLeft);
        g.setColour(white);
        g.setFont(16.0f);
        blp::drawText(g, juce::String(juce::roundToInt(slider.getValue())) + " %", x + 6, y + 20, w - 30, 24, juce::Justification::centredLeft);
    }
};

class Editor final : public blp::ThemedEditor, private juce::Timer
{
public:
    Editor(juce::AudioProcessor& owner, juce::AudioProcessorValueTreeState& state)
        : blp::ThemedEditor(owner)
    {
        setTitle(description);
        setDescription(description);
        for (size_t i = 0; i < sliders.size(); ++i)
        {
            auto& slider = sliders[i];
            slider.setName(names[i]);
            slider.setTitle(names[i]);
            slider.setSliderStyle(juce::Slider::LinearVertical);
            slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slider.setLookAndFeel(&look);
            slider.setDoubleClickReturnValue(true, defaults[i]);
            addAndMakeVisible(slider);
            bindings[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, ids[i], slider);
        }
        addAndMakeVisible(bypass);
        bypassBinding = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, "bypass", bypass);
        setSize(276, 270);
        startTimerHz(30);
    }

    ~Editor() override
    {
        stopTimer();
        for (auto& slider : sliders)
            slider.setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(blp::canvas);
        g.setColour(juce::Colour(0xfff6f5f7));
        g.setFont(juce::FontOptions(16.0f));
        
        g.setColour(juce::Colour(0xff323238));
        g.drawHorizontalLine(194, 16, static_cast<float>(getWidth() - 16));
        g.drawVerticalLine(getWidth() / 2, 204, 250);
        blp::paintHeader(g, getWidth(), "BLP Studio Reverb");
}

    void resized() override
    {
        sliders[0].setBounds(13, 38, getWidth() - 26, 155);
        bypass.setBounds(blp::bypassBounds(getWidth()));
        const auto controlWidth = getWidth() / 2 - 26;
        sliders[1].setBounds(16, 197, controlWidth, 60);
        sliders[2].setBounds(getWidth() / 2 + 10, 197, controlWidth, 60);
    }

private:
    void timerCallback() override
    {
        if (sliders[0].getValue() > 0.0)
            sliders[0].repaint();
    }

    StudioLook look;
    blp::BypassButton bypass;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassBinding;
    std::array<juce::Slider, 3> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 3> bindings;
};

class StudioReverb final : public juce::AudioProcessor
{
public:
    StudioReverb()
        : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          state(*this, nullptr, "StudioReverb", makeParameters())
    {
        for (size_t i = 0; i < values.size(); ++i)
            values[i] = state.getRawParameterValue(ids[i]);
    }

    void prepareToPlay(double rate, int) override
    {
        room.prepare(rate);
        active.reset(rate, 0.020);
        active.setCurrentAndTargetValue(bypassAmount());
        for (size_t i = 0; i < smooth.size(); ++i)
        {
            smooth[i].reset(rate, i == 1 ? 0.065 : 0.020);
            smooth[i].setCurrentAndTargetValue(values[i]->load() / 100.0f);
        }
    }

    bool isBusesLayoutSupported(const BusesLayout& layout) const override
    {
        const auto output = layout.getMainOutputChannelSet();
        return output == layout.getMainInputChannelSet()
            && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
    }

    void processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        for (size_t i = 0; i < smooth.size(); ++i)
            smooth[i].setTargetValue(values[i]->load() / 100.0f);

        active.setTargetValue(bypassAmount());
        auto* left = audio.getWritePointer(0);
        auto* right = audio.getNumChannels() > 1 ? audio.getWritePointer(1) : nullptr;
        for (int i = 0; i < audio.getNumSamples(); ++i)
        {
            const float mix = smooth[0].getNextValue() * active.getNextValue();
            const auto wet = room.tick(left[i], right != nullptr ? right[i] : left[i],
                                       smooth[1].getNextValue(), smooth[2].getNextValue());
            if (mix == 0.0f)
                continue;
            const float dry = 1.0f - mix;
            if (right != nullptr)
            {
                left[i] = dry * left[i] + mix * wet[0];
                right[i] = dry * right[i] + mix * wet[1];
            }
            else
                left[i] = dry * left[i] + mix * 0.5f * (wet[0] + wet[1]);
        }
    }

    void reset() override { room.clear(); }
    void releaseResources() override {}
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return new Editor(*this, state); }

    void getStateInformation(juce::MemoryBlock& data) override
    {
        if (auto xml = state.copyState().createXml())
            copyXmlToBinary(*xml, data);
    }

    void setStateInformation(const void* data, int bytes) override
    {
        if (auto xml = getXmlFromBinary(data, bytes); xml && xml->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        for (size_t i = 0; i < ids.size(); ++i)
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID(ids[i], 1), names[i], juce::NormalisableRange<float>(0, 100), defaults[i],
                juce::AudioParameterFloatAttributes().withLabel("%")
                    .withStringFromValueFunction([](float value, int) { return juce::String(value, 1); })));
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("bypass", 1), "Bypass", false));
        return layout;
    }

    float bypassAmount() const { return 1.0f - bypassValue->load(); }

    juce::AudioProcessorValueTreeState state;
    std::atomic<float>* bypassValue = state.getRawParameterValue("bypass");
    juce::SmoothedValue<float> active;
    std::array<std::atomic<float>*, 3> values {};
    std::array<juce::SmoothedValue<float>, 3> smooth;
    Room room;
};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StudioReverb();
}






