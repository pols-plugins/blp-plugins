#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeParameters()
{
    using namespace juce;
    NormalisableRange<float> toneRange{
        250.0f, 16000.0f,
        [](float lo, float hi, float t) { return lo * std::pow(hi / lo, t); },
        [](float lo, float hi, float hz) { return std::log(hz / lo) / std::log(hi / lo); },
        [](float, float, float hz) { return std::round(hz); }};
    AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<AudioParameterFloat>(ParameterID{"drive", 1}, "Drive",
                                                    NormalisableRange<float>{0.0f, 10.0f, 0.1f}, 3.5f),
               std::make_unique<AudioParameterFloat>(ParameterID{"tone", 1}, "Tone", toneRange, 1800.0f,
                                                    AudioParameterFloatAttributes().withLabel("Hz")),
               std::make_unique<AudioParameterFloat>(ParameterID{"volume", 1}, "Volume",
                                                    NormalisableRange<float>{0.0f, 10.0f, 0.1f}, 4.5f,
                                                    AudioParameterFloatAttributes().withLabel("dB")),
               std::make_unique<AudioParameterBool>(ParameterID{"bypass", 1}, "Bypass", false));
    return layout;
}
}

DriverProcessor::DriverProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BLPDriverState", makeParameters()),
      driveParam(state.getRawParameterValue("drive")),
      toneParam(state.getRawParameterValue("tone")),
      volumeParam(state.getRawParameterValue("volume")),
      bypassParam(state.getRawParameterValue("bypass"))
{
}

DriverProcessor::Settings DriverProcessor::readSettings() const noexcept
{
    return {driveParam->load(), toneParam->load(), volumeParam->load(), bypassParam->load()};
}

bool DriverProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto out = layout.getMainOutputChannelSet();
    return out == layout.getMainInputChannelSet()
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void DriverProcessor::prepareToPlay(double sampleRate, int)
{
    rate = static_cast<float>(sampleRate);
    channels = {};
    const auto values = readSettings();
    drive.reset(sampleRate, 0.02);
    toneCoefficient.reset(sampleRate, 0.02);
    volumeGain.reset(sampleRate, 0.02);
    wet.reset(sampleRate, 0.01);
    drive.setCurrentAndTargetValue(values.drive);
    toneCoefficient.setCurrentAndTargetValue(Signal::toneCoefficient(values.tone, rate));
    volumeGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(values.volume));
    wet.setCurrentAndTargetValue(1.0f - values.bypass);
}

void DriverProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    render(buffer, false);
}

void DriverProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    render(buffer, true);
}

void DriverProcessor::render(juce::AudioBuffer<float>& buffer, bool hostBypassed)
{
    juce::ScopedNoDenormals noDenormals;
    const auto values = readSettings();
    drive.setTargetValue(values.drive);
    toneCoefficient.setTargetValue(Signal::toneCoefficient(values.tone, rate));
    volumeGain.setTargetValue(juce::Decibels::decibelsToGain(values.volume));
    wet.setTargetValue(hostBypassed ? 0.0f : 1.0f - values.bypass);
    auto samples = buffer.getArrayOfWritePointers();
    const auto count = juce::jmin(buffer.getNumChannels(), static_cast<int>(channels.size()));

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto d = drive.getNextValue();
        const auto amount = juce::jmin(1.0f, d / 3.5f);
        const auto gain = Signal::inputGain(d);
        const auto coefficient = toneCoefficient.getNextValue();
        const auto output = volumeGain.getNextValue();
        const auto mix = wet.getNextValue();

        for (int ch = 0; ch < count; ++ch)
        {
            const auto dry = samples[ch][i];
            const auto shaped = channels[static_cast<size_t>(ch)].process(dry, amount, gain, coefficient, output);
            if (mix > 0.0f)
                samples[ch][i] = dry + mix * (shaped - dry);
        }
    }
}

juce::AudioProcessorParameter* DriverProcessor::getBypassParameter() const
{
    return state.getParameter("bypass");
}

void DriverProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (const auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void DriverProcessor::setStateInformation(const void* bytes, int size)
{
    if (const auto xml = getXmlFromBinary(bytes, size))
        if (xml->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* DriverProcessor::createEditor() { return new DriverEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DriverProcessor; }
