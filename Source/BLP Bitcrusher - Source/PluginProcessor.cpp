#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
constexpr float maxHold = 96.0f;

float quantize(float sample, float halfLevels)
{
    // Keep digital silence silent. Nonzero samples use a signed mid-rise quantizer.
    if (sample == 0.0f)
        return 0.0f;

    const auto scaled = juce::jlimit(-1.0f, 1.0f, sample) * halfLevels;
    const auto code = juce::jlimit(-halfLevels, halfLevels - 1.0f, std::floor(scaled));
    return (code + 0.5f) / halfLevels;
}
}

BitcrusherProcessor::BitcrusherProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BLPBitcrusherState", makeParameters()),
      intensityValue(parameters.getRawParameterValue("intensity")),
      bitsValue(parameters.getRawParameterValue("bits")),
      mixValue(parameters.getRawParameterValue("mix")),
      bypassValue(parameters.getRawParameterValue("bypass"))
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BitcrusherProcessor::makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto percent = juce::AudioParameterIntAttributes().withLabel("%");
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "intensity", 1 },
                                                         "Intensity", 0, 100, 49, percent),
               std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "bits", 1 },
                                                         "Bits", 1, 16, 16),
               std::make_unique<juce::AudioParameterInt>(juce::ParameterID { "mix", 1 },
                                                         "Mix", 0, 100, 70, percent),
               std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 },
                                                          "Bypass", false));
    return layout;
}

void BitcrusherProcessor::prepareToPlay(double sampleRate, int)
{
    intensity.reset(sampleRate, 0.02);
    bits.reset(sampleRate, 0.02);
    mix.reset(sampleRate, 0.015);
    active.reset(sampleRate, 0.01);
    intensity.setCurrentAndTargetValue(intensityValue->load() * 0.01f);
    bits.setCurrentAndTargetValue(bitsValue->load());
    mix.setCurrentAndTargetValue(mixValue->load() * 0.01f);
    active.setCurrentAndTargetValue(bypassValue->load() >= 0.5f ? 0.0f : 1.0f);
    phase = 1.0f;
    held.fill(0.0f);
}

bool BitcrusherProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return layout.getMainInputChannelSet() == output
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void BitcrusherProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    render(audio, false);
}

void BitcrusherProcessor::processBlockBypassed(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    render(audio, true);
}

void BitcrusherProcessor::render(juce::AudioBuffer<float>& audio, bool hostBypass)
{
    juce::ScopedNoDenormals noDenormals;
    intensity.setTargetValue(intensityValue->load(std::memory_order_relaxed) * 0.01f);
    bits.setTargetValue(bitsValue->load(std::memory_order_relaxed));
    mix.setTargetValue(mixValue->load(std::memory_order_relaxed) * 0.01f);
    active.setTargetValue(hostBypass || bypassValue->load(std::memory_order_relaxed) >= 0.5f
                              ? 0.0f : 1.0f);

    auto* const* channels = audio.getArrayOfWritePointers();
    const auto count = juce::jmin(audio.getNumChannels(), static_cast<int>(held.size()));
    for (int i = 0; i < audio.getNumSamples(); ++i)
    {
        const auto amount = intensity.getNextValue();
        const auto depth = bits.getNextValue();
        const auto wet = mix.getNextValue() * active.getNextValue();
        phase += 1.0f / (1.0f + (maxHold - 1.0f) * amount * amount * amount);
        const auto capture = amount == 0.0f || phase >= 1.0f;
        if (capture)
            phase -= std::floor(phase);

        // Blend adjacent quantizers while changing depth so every settled setting
        // still uses its exact integer bit depth.
        const auto lowerDepth = static_cast<int>(depth);
        const auto halfLevels = static_cast<float>(1 << (lowerDepth - 1));
        const auto fraction = depth - static_cast<float>(lowerDepth);
        for (int ch = 0; ch < count; ++ch)
        {
            const auto dry = channels[ch][i];
            if (capture)
                held[static_cast<size_t>(ch)] = dry;

            if (wet == 0.0f)
                continue;

            const auto input = held[static_cast<size_t>(ch)];
            const auto coarse = quantize(input, halfLevels);
            const auto crushed = fraction == 0.0f ? coarse
                : coarse + fraction * (quantize(input, halfLevels * 2.0f) - coarse);
            channels[ch][i] = dry + wet * (crushed - dry);
        }
    }
}

juce::AudioProcessorParameter* BitcrusherProcessor::getBypassParameter() const
{
    return parameters.getParameter("bypass");
}

void BitcrusherProcessor::getStateInformation(juce::MemoryBlock& block)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, block);
}

void BitcrusherProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* BitcrusherProcessor::createEditor()
{
    return new BitcrusherEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BitcrusherProcessor;
}

