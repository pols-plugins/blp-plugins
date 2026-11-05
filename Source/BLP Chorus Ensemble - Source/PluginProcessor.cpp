#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (size_t i = 0; i < Chorus::defaults.size(); ++i)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { Chorus::ids[i], 1 }, Chorus::names[i],
            juce::NormalisableRange<float> { 0.f, 10.f }, Chorus::defaults[i], juce::AudioParameterFloatAttributes().withStringFromValueFunction(
                [](float value, int) { return juce::String(value, 1); })));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}
}

Chorus::Chorus()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "BLPChorusState", makeLayout())
{
    for (size_t i = 0; i < parameters.size(); ++i)
        parameters[i] = state.getRawParameterValue(ids[i]);
}

bool Chorus::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto input = layout.getMainInputChannelSet();
    const auto output = layout.getMainOutputChannelSet();
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();
    return (input == mono && (output == mono || output == stereo)) || (input == stereo && output == stereo);
}

void Chorus::updateTargets(bool bypass)
{
    const auto value = [this](size_t index) { return parameters[index]->load() * 0.1f; };
    const auto coefficient = [this](float hz)
    {
        return 1.f - std::exp(-juce::MathConstants<float>::twoPi * hz / rate);
    };
    smooth[0].setTargetValue(7.f * value(0));
    smooth[1].setTargetValue(bypass || value(5) > 0.f ? 0.f : 0.8f * value(1));
    smooth[2].setTargetValue(0.05f * std::pow(160.f, value(2)));
    smooth[3].setTargetValue(coefficient(600.f * std::pow(0.025f, value(3))));
    smooth[4].setTargetValue(coefficient(std::min(rate * 0.45f, 700.f * std::pow(28.f, value(4)))));
}

void Chorus::prepareToPlay(double sampleRate, int)
{
    rate = static_cast<float>(sampleRate);
    for (auto& line : delay)
        line.assign(static_cast<size_t>(std::ceil(rate * 0.035f)) + 2, 0.f);
    bassLow.fill(0.f);
    trebleLow.fill(0.f);
    position = 0;
    phase = 0.0;
    updateTargets(false);
    for (auto& parameter : smooth)
        parameter.reset(sampleRate, 0.03);
}

void Chorus::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    render(audio, false);
}

void Chorus::processBlockBypassed(juce::AudioBuffer<float>& audio, juce::MidiBuffer&)
{
    render(audio, true);
}

void Chorus::render(juce::AudioBuffer<float>& audio, bool bypass)
{
    juce::ScopedNoDenormals noDenormals;
    const int length = static_cast<int>(delay[0].size());
    if (length == 0)
        return;
    updateTargets(bypass);
    constexpr double turn = juce::MathConstants<double>::twoPi;
    constexpr std::array baseMs { 11.f, 16.f, 22.f };
    auto channels = audio.getArrayOfWritePointers();
    const int inputs = getTotalNumInputChannels();
    const int outputs = getTotalNumOutputChannels();
    for (int sample = 0; sample < audio.getNumSamples(); ++sample)
    {
        const float depth = smooth[0].getNextValue();
        const float mix = smooth[1].getNextValue();
        const float speed = smooth[2].getNextValue();
        const float hp = smooth[3].getNextValue();
        const float lp = smooth[4].getNextValue();
        const float left = channels[0][sample];
        const std::array dry { left, inputs == 2 ? channels[1][sample] : left };
        for (int channel = 0; channel < outputs; ++channel)
        {
            auto& line = delay[channel];
            line[position] = dry[channel];
            float wet = 0.f;
            for (size_t voice = 0; voice < baseMs.size(); ++voice)
            {
                const double angle = phase + turn * static_cast<double>(voice) / 3.0 + channel * 1.05;
                const float ms = baseMs[voice] + depth * static_cast<float>(std::sin(angle));
                float read = static_cast<float>(position) - ms * rate * 0.001f;
                if (read < 0.f)
                    read += static_cast<float>(length);
                const int index = static_cast<int>(read);
                const int next = index + 1 == length ? 0 : index + 1;
                wet += line[index] + (read - static_cast<float>(index)) * (line[next] - line[index]);
            }
            wet /= 3.f;
            auto& low = bassLow[channel];
            auto& filtered = trebleLow[channel];
            low += hp * (wet - low);
            filtered += lp * (wet - low - filtered);
            channels[channel][sample] = dry[channel] + mix * (filtered - dry[channel]);
        }
        if (++position == length)
            position = 0;
        phase += turn * speed / rate;
        if (phase >= turn)
            phase -= turn;
    }
}

void Chorus::getStateInformation(juce::MemoryBlock& data)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary(*xml, data);
}

void Chorus::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(state.state.getType()))
        state.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* Chorus::createEditor() { return new ChorusEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Chorus; }

