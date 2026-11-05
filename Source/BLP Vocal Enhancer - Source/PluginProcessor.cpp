#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 }, "Mix", juce::NormalisableRange<float> { 0.0f, 100.0f },
        50.0f, juce::AudioParameterFloatAttributes().withLabel("%")));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "Warm", "Standard", "Modern", "General" }, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}
}

VocalEnhancerProcessor::VocalEnhancerProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BLPVocalEnhancerState", makeParameters()),
      mix(parameters.getRawParameterValue("mix")),
      mode(parameters.getRawParameterValue("mode")),
      bypass(parameters.getRawParameterValue("bypass"))
{
}

bool VocalEnhancerProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return output == layout.getMainInputChannelSet()
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void VocalEnhancerProcessor::prepareToPlay(double rate, int)
{
    dsp.prepare(rate, mix->load() * 0.01f, static_cast<int>(mode->load()), bypass->load() >= 0.5f);
}

void VocalEnhancerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    dsp.process(buffer, mix->load() * 0.01f, static_cast<int>(mode->load()), bypass->load() >= 0.5f);
}

void VocalEnhancerProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    dsp.process(buffer, mix->load() * 0.01f, static_cast<int>(mode->load()), true);
}

juce::AudioProcessorParameter* VocalEnhancerProcessor::getBypassParameter() const
{
    return parameters.getParameter("bypass");
}

juce::AudioProcessorEditor* VocalEnhancerProcessor::createEditor()
{
    return new VocalEnhancerEditor(*this);
}

void VocalEnhancerProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void VocalEnhancerProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalEnhancerProcessor();
}
