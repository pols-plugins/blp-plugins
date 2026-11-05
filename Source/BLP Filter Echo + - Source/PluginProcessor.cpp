#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
const StringArray EchoProcessor::divisions { "1/2", "1/4", "1/4T", "1/8 dot", "1/8", "1/8T", "1/16 dot", "1/16", "1/16T" };
namespace
{
constexpr double beats[] { 2, 1, 2.0/3, 0.75, 0.5, 1.0/3, 0.375, 0.25, 1.0/6 };
constexpr float feedbackLimit = 0.95f;
constexpr const char* ids[] { "feedback", "mode", "division", "ms", "cutoff", "mix", "bypass" };

AudioProcessorValueTreeState::ParameterLayout parameters()
{
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto slider = [&layout](const char* id, const char* name, float min, float max,
                            float step, float skew, float initial, const char* unit)
    {
        layout.add(std::make_unique<AudioParameterFloat>(ParameterID { id, 1 }, name,
            NormalisableRange<float> { min, max, step, skew }, initial,
            AudioParameterFloatAttributes().withLabel(unit)));
    };
    slider("feedback", "Feedback", 0, 100, 1, 1, 24, "%");
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "mode", 1 }, "Time Mode", StringArray { "Beat", "ms" }, 0));
    layout.add(std::make_unique<AudioParameterChoice>(ParameterID { "division", 1 }, "Beat Division", EchoProcessor::divisions, 1));
    slider("ms", "Delay Time", 0, 1000, 1, 1, 200, "ms");
    slider("cutoff", "Cutoff", 300, 12000, 1, 0.45f, 12000, "Hz");
    slider("mix", "Mix", 0, 100, 1, 1, 40, "%");
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}
}

EchoProcessor::EchoProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", AudioChannelSet::stereo(), true)
                                     .withOutput("Output", AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "FilterEchoState", parameters())
{
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = state.getRawParameterValue(ids[i]);
}

bool EchoProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return output == layout.getMainInputChannelSet()
        && (output == AudioChannelSet::mono() || output == AudioChannelSet::stereo());
}

void EchoProcessor::prepareToPlay(double sampleRate, int)
{
    rate = sampleRate;
    tempo = 120.0;
    history.setSize(getTotalNumOutputChannels(), int(std::ceil(12.0 * rate)) + 2);
    fadeLength = jmax(1, roundToInt(0.025 * rate));
    feedbackGain.reset(rate, 0.02);
    filterGain.reset(rate, 0.02);
    wetGain.reset(rate, 0.015);
    reset();
}

void EchoProcessor::reset()
{
    history.clear();
    filtered.fill(0.0f);
    head = fadeRemaining = 0;
    needsInitialTap = true;
}

double EchoProcessor::delaySamples() const
{
    const auto seconds = value(mode) > 0.5f ? value(milliseconds) * 0.001
        : beats[roundToInt(value(division))] * 60.0 / tempo;
    return jlimit(0.0, 12.0, seconds) * rate;
}

void EchoProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&) { render(buffer, false); }
void EchoProcessor::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer&) { render(buffer, true); }

void EchoProcessor::render(AudioBuffer<float>& buffer, bool hostBypassed)
{
    ScopedNoDenormals noDenormals;
    if (auto* host = getPlayHead())
        if (auto position = host->getPosition())
            if (auto bpm = position->getBpm(); bpm && std::isfinite(*bpm) && *bpm > 0.0)
                tempo = *bpm;

    const double target = delaySamples();
    const auto gain = value(feedback) * (feedbackLimit / 100.0f);
    const auto coefficient = float(-std::expm1(-MathConstants<double>::twoPi
        * jmin(double(value(cutoff)), 0.49 * rate) / rate));
    const auto wet = hostBypassed || value(bypass) > 0.5f ? 0.0f : value(mix) * 0.01f;
    if (needsInitialTap)
    {
        from = to = target;
        feedbackGain.setCurrentAndTargetValue(gain);
        filterGain.setCurrentAndTargetValue(coefficient);
        wetGain.setCurrentAndTargetValue(wet);
        needsInitialTap = false;
    }
    feedbackGain.setTargetValue(gain);
    filterGain.setTargetValue(coefficient);
    wetGain.setTargetValue(wet);

    const int length = history.getNumSamples();
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        if (fadeRemaining == 0 && std::abs(target - to) > 0.01)
        {
            from = to;
            to = target;
            fadeRemaining = fadeLength;
        }
        const float blend = 1.0f - float(fadeRemaining) / float(fadeLength);
        const float feedbackAmount = feedbackGain.getNextValue();
        const float lowpass = filterGain.getNextValue();
        const float mixAmount = wetGain.getNextValue();

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* ring = history.getWritePointer(channel);
            const float dry = buffer.getSample(channel, sample);
            auto tap = [&](double delay)
            {
                if (delay < 1.0)
                    return dry + float(delay) * (ring[head == 0 ? length - 1 : head - 1] - dry);
                double index = head - delay;
                if (index < 0.0)
                    index += length;
                const int first = int(index);
                const int next = first + 1 == length ? 0 : first + 1;
                return ring[first] + float(index - first) * (ring[next] - ring[first]);
            };
            const float delayed = tap(to);
            const float read = fadeRemaining > 0 ? delayed * blend + tap(from) * (1.0f - blend) : delayed;
            auto& memory = filtered[size_t(channel)];
            memory += lowpass * (read - memory);
            ring[head] = dry + feedbackAmount * memory;
            if (mixAmount > 0.0f)
                buffer.setSample(channel, sample, dry + mixAmount * (memory - dry));
        }
        if (++head == length)
            head = 0;
        if (fadeRemaining > 0)
            --fadeRemaining;
    }
}

double EchoProcessor::getTailLengthSeconds() const
{
    const double gain = value(feedback) * (feedbackLimit / 100.0);
    return 12.0 * (gain > 0.0 ? 1.0 + std::ceil(std::log(0.001) / std::log(gain)) : 1.0);
}

AudioProcessorParameter* EchoProcessor::getBypassParameter() const { return state.getParameter("bypass"); }
AudioProcessorEditor* EchoProcessor::createEditor() { return new EchoEditor(*this); }

void EchoProcessor::getStateInformation(MemoryBlock& output)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, output);
}

void EchoProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType()))
        state.replaceState(ValueTree::fromXml(*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EchoProcessor(); }
