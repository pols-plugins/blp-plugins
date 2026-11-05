#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout parameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto percent = [](const char* id, const char* name, float initial)
    {
        auto display = juce::AudioParameterFloatAttributes().withLabel("%")
            .withStringFromValueFunction([](float value, int) { return juce::String(value, 1); })
            .withValueFromStringFunction([](const juce::String& value) { return value.getFloatValue(); });
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f},
            initial, display);
    };
    layout.add(percent("speed", "Speed", 60.0f), percent("depth", "Depth", 100.0f),
               std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass", 1}, "Bypass", false));
    return layout;
}
}

ChorusProcessor::ChorusProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "ChorusState", parameters()),
      speed(state.getRawParameterValue("speed")),
      depth(state.getRawParameterValue("depth")),
      bypass(state.getRawParameterValue("bypass"))
{
}

bool ChorusProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto in = layout.getMainInputChannelSet();
    const auto out = layout.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && (out == in || out == juce::AudioChannelSet::stereo());
}

void ChorusProcessor::prepareToPlay(double newRate, int)
{
    sampleRate = newRate;
    delay.setSize(2, static_cast<int>(std::ceil(0.023 * sampleRate)) + 2);
    delay.clear();
    writeIndex = 0;
    phase = 0.0;
    for (auto* smoother : {&frequency, &excursion, &wet})
        smoother->reset(sampleRate, 0.03);
    frequency.setCurrentAndTargetValue(rate(speed->load() * 0.01f));
    excursion.setCurrentAndTargetValue(depth->load() * 0.00008f);
    wet.setCurrentAndTargetValue(bypass->load() >= 0.5f ? 0.0f : 0.5f);
}

void ChorusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    constexpr auto turn = juce::MathConstants<double>::twoPi;
    frequency.setTargetValue(rate(speed->load() * 0.01f));
    excursion.setTargetValue(depth->load() * 0.00008f);
    wet.setTargetValue(bypass->load() >= 0.5f ? 0.0f : 0.5f);

    if (getTotalNumInputChannels() == 1 && getTotalNumOutputChannels() == 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

    const int size = delay.getNumSamples();
    auto* output = buffer.getArrayOfWritePointers();
    auto* history = delay.getArrayOfWritePointers();
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto swing = excursion.getNextValue();
        const auto mix = wet.getNextValue();
        for (int ch = 0; ch < getTotalNumOutputChannels(); ++ch)
        {
            const float dry = output[ch][sample];
            history[ch][writeIndex] = dry;
            if (mix == 0.0f)
                continue;

            double read = writeIndex - sampleRate * (0.015 + swing * std::sin(phase + ch * turn / 4.0));
            if (read < 0.0)
                read += size;
            const int first = static_cast<int>(read);
            const int second = first + 1 == size ? 0 : first + 1;
            const auto fraction = static_cast<float>(read - first);
            const float voice = history[ch][first] + fraction * (history[ch][second] - history[ch][first]);
            // Normalize the sum so an in-phase wet voice cannot raise peak gain.
            output[ch][sample] = (dry + mix * voice) / (1.0f + mix);
        }
        if (++writeIndex == size)
            writeIndex = 0;
        phase += frequency.getNextValue() * turn / sampleRate;
        if (phase >= turn)
            phase -= turn;
    }
}

void ChorusProcessor::getStateInformation(juce::MemoryBlock& data)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void ChorusProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(state.state.getType()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* ChorusProcessor::createEditor() { return new ChorusEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChorusProcessor(); }
