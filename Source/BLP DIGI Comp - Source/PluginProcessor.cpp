#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using namespace juce;
    constexpr std::array minimum { 1.f, 1.f, -60.f, 1.f, 0.f };
    constexpr std::array maximum { 1000.f, 1000.f, 0.f, 20.f, 20.f };
    constexpr std::array units { " ms", " ms", " dB", ":1", " dB" };
    AudioProcessorValueTreeState::ParameterLayout layout;
    for (size_t i = 0; i < Compressor::names.size(); ++i)
    {
        NormalisableRange<float> range { minimum[i], maximum[i], i < 2 ? 1.f : 0.1f };
        if (i < 2)
            range.setSkewForCentre(100.f);

        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID { Compressor::ids[i], 1 }, Compressor::names[i], range,
            Compressor::defaults[i], AudioParameterFloatAttributes()
                .withStringFromValueFunction([i, unit = units[i]](float value, int)
                {
                    return String(value, i < 2 ? 0 : 1) + unit;
                })
                .withValueFromStringFunction([](const String& value) { return value.getFloatValue(); })));
    }
    layout.add(std::make_unique<AudioParameterBool>(ParameterID { Compressor::ids[5], 1 }, "Bypass", false));
    return layout;
}
}

Compressor::Compressor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BLPDIGIComp", makeLayout())
{
    for (size_t i = 0; i < parameters.size(); ++i)
        parameters[i] = state.getRawParameterValue(ids[i]);
}

bool Compressor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return layout.getMainInputChannelSet() == output
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void Compressor::prepareToPlay(double sampleRate, int)
{
    timeScale = static_cast<float>(-1000.0 / sampleRate);
    reduction = 0.f;
    for (size_t i = 0; i < smooth.size(); ++i)
    {
        smooth[i].reset(sampleRate, 0.015);
        smooth[i].setCurrentAndTargetValue(parameters[i]->load());
    }
    wet.reset(sampleRate, 0.005);
    wet.setCurrentAndTargetValue(parameters[5]->load() >= 0.5f ? 0.f : 1.f);
}

void Compressor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    render(buffer, false);
}

void Compressor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    render(buffer, true);
}

void Compressor::render(juce::AudioBuffer<float>& buffer, bool hostBypass)
{
    const juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() == 0)
        return;

    for (size_t i = 0; i < smooth.size(); ++i)
        smooth[i].setTargetValue(parameters[i]->load());
    wet.setTargetValue(hostBypass || parameters[5]->load() >= 0.5f ? 0.f : 1.f);

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    auto attack = std::exp(timeScale / smooth[0].getCurrentValue());
    auto release = std::exp(timeScale / smooth[1].getCurrentValue());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (smooth[0].isSmoothing())
            attack = std::exp(timeScale / smooth[0].getNextValue());
        if (smooth[1].isSmoothing())
            release = std::exp(timeScale / smooth[1].getNextValue());

        const auto peak = std::max(std::abs(left[i]), right ? std::abs(right[i]) : 0.f);
        const auto over = juce::Decibels::gainToDecibels(peak, -120.f) - smooth[2].getNextValue();
        const auto slope = 1.f - 1.f / smooth[3].getNextValue();
        const auto knee = smooth[4].getNextValue();
        const auto halfKnee = knee * 0.5f;
        auto target = 0.f;
        if (over >= halfKnee)
            target = slope * over;
        else if (over > -halfKnee)
        {
            const auto offset = over + halfKnee;
            target = slope * offset * offset / (2.f * knee);
        }

        const auto coefficient = target > reduction ? attack : release;
        reduction = target + coefficient * (reduction - target);
        const auto mix = wet.getNextValue();
        if (mix > 0.f)
        {
            const auto compressedGain = std::pow(10.f, -reduction * 0.05f);
            const auto gain = 1.f + mix * (compressedGain - 1.f);
            left[i] *= gain;
            if (right)
                right[i] *= gain;
        }
    }
}

void Compressor::getStateInformation(juce::MemoryBlock& destination)
{
    if (const auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void Compressor::setStateInformation(const void* data, int size)
{
    if (const auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType()))
        state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* Compressor::createEditor()
{
    return new CompressorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Compressor();
}
