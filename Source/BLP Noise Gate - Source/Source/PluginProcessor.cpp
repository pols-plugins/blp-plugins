#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float minusInfinityDb = -100.0f;

    float dbToGain (float db) noexcept
    {
        return juce::Decibels::decibelsToGain (db, minusInfinityDb);
    }

    float smoothingCoefficient (float milliseconds, double sampleRate) noexcept
    {
        if (milliseconds <= 0.0f)
            return 0.0f;

        const auto seconds = static_cast<double> (milliseconds) * 0.001;
        return static_cast<float> (std::exp (-1.0 / (seconds * sampleRate)));
    }
}

NoiseGateAudioProcessor::NoiseGateAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
NoiseGateAudioProcessor::createParameterLayout()
{
    using Parameter = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Parameter> ("threshold", "Threshold",
        juce::NormalisableRange<float> (-72.0f, -18.0f, 0.1f), -38.0f, "dB"));
    layout.add (std::make_unique<Parameter> ("attack", "Attack",
        juce::NormalisableRange<float> (1.0f, 150.0f, 0.1f, 0.45f), 5.0f, "ms"));
    layout.add (std::make_unique<Parameter> ("release", "Release",
        juce::NormalisableRange<float> (100.0f, 800.0f, 0.1f, 0.45f), 150.0f, "ms"));
    layout.add (std::make_unique<juce::AudioParameterBool> ("bypass", "Bypass", false));

    return layout;
}

bool NoiseGateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return input == juce::AudioChannelSet::mono()
        || input == juce::AudioChannelSet::stereo();
}

void NoiseGateAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    detectorEnvelope = 0.0f;
    gateGain = 1.0f;
}

void NoiseGateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = getTotalNumInputChannels();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const bool bypassed = parameters.getRawParameterValue ("bypass")->load() > 0.5f;

    if (bypassed)
        return; // True bypass: leave the input buffer untouched.

    const float threshold = parameters.getRawParameterValue ("threshold")->load();
    const float attackMs = parameters.getRawParameterValue ("attack")->load();
    const float releaseMs = parameters.getRawParameterValue ("release")->load();

    const float attackCoeff = smoothingCoefficient (attackMs, currentSampleRate);
    const float releaseCoeff = smoothingCoefficient (releaseMs, currentSampleRate);
    const float closedGain = dbToGain (-45.0f);
    const float thresholdGain = dbToGain (threshold);

    auto* const* channels = buffer.getArrayOfWritePointers();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Stereo-linked peak detection ensures
        // that either channel can open the gate for both channels.
        float detectorPeak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            detectorPeak = juce::jmax (detectorPeak, std::abs (channels[ch][sample]));

        // Peak envelope follower: fast attack, slower release.
        const float detectorCoeff = detectorPeak > detectorEnvelope ? attackCoeff : releaseCoeff;
        detectorEnvelope = detectorPeak + detectorCoeff * (detectorEnvelope - detectorPeak);

        const bool shouldOpen = detectorEnvelope >= thresholdGain;
        const float targetGain = shouldOpen ? 1.0f : closedGain;

        // Smooth gain independently to prevent clicks. Opening follows attack;
        // closing follows release. The gate closes to a fixed attenuation floor.
        const float gainCoeff = targetGain > gateGain ? attackCoeff : releaseCoeff;
        gateGain = targetGain + gainCoeff * (gateGain - targetGain);

        for (int ch = 0; ch < numChannels; ++ch)
            channels[ch][sample] *= gateGain;
    }
}

juce::AudioProcessorEditor* NoiseGateAudioProcessor::createEditor()
{
    return new NoiseGateAudioProcessorEditor (*this);
}

void NoiseGateAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NoiseGateAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoiseGateAudioProcessor();
}
