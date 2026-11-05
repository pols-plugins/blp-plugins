#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto add = [&layout](const char* id, const char* name, float max, float initial, bool percent)
    {
        auto attributes = juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction([percent](float value, int limit)
            {
                auto label = juce::String(value, 2).trimCharactersAtEnd("0").trimCharactersAtEnd(".");
                if (percent)
                    label += "%";
                return limit > 0 ? label.substring(0, limit) : label;
            })
            .withValueFromStringFunction([](const juce::String& value) { return value.getFloatValue(); });
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>{0.0f, max}, initial, attributes));
    };
    add("spread", "Spread", 10.0f, 5.7f, false);
    add("low", "Low", 100.0f, 100.0f, true);
    add("high", "High", 100.0f, 100.0f, true);
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false));
    return layout;
}
}

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BLPStereowiseState", makeLayout()),
      spread(parameters.getRawParameterValue("spread")),
      low(parameters.getRawParameterValue("low")),
      high(parameters.getRawParameterValue("high")),
      bypass(parameters.getRawParameterValue("bypass"))
{}

void PluginProcessor::prepareToPlay(double rate, int)
{
    width.prepare(rate, spread->load(), low->load(), high->load(), bypass->load() > 0.5f);
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return output == layout.getMainInputChannelSet()
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    width.process(buffer, spread->load(), low->load(), high->load(), bypass->load() > 0.5f);
}

void PluginProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    width.process(buffer, spread->load(), low->load(), high->load(), true);
}

juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const
{
    return parameters.getParameter("bypass");
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void PluginProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
