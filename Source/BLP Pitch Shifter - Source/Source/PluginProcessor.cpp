#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout parameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    result.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"input", 1},
        "Input Type", juce::StringArray{"Vocal", "General", "Percussion"}, 0));
    result.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"shift", 1},
        "Shift", -12, 12, -5, juce::AudioParameterIntAttributes().withLabel("st")));
    result.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1},
        "Mix", juce::NormalisableRange<float>{0.0f, 100.0f}, 100.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));
    result.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1},
        "Bypass", false));
    return result;
}
}

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BLPState", parameters())
{
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    auto output = layout.getMainOutputChannelSet();
    return output == layout.getMainInputChannelSet()
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void PluginProcessor::prepareToPlay(double rate, int)
{
    setLatencySamples(shifter.prepare(rate, getTotalNumInputChannels(), shift->load(),
        mix->load() / 100.0f, int(mode->load()), bypass->load() >= 0.5f));
}

void PluginProcessor::render(juce::AudioBuffer<float>& buffer, bool hostBypass)
{
    juce::ScopedNoDenormals guard;
    shifter.process(buffer, shift->load(), mix->load() / 100.0f,
                    int(mode->load()), hostBypass || bypass->load() >= 0.5f);
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    render(buffer, false);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    render(buffer, true);
}

juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const
{
    return state.getParameter("bypass");
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void PluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor; }

