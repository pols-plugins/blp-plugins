#include "PluginProcessor.h"
#include "PluginEditor.h"

const juce::StringArray Processor::keys {"C","C#/Db","D","D#/Eb","E","F","F#/Gb","G","G#/Ab","A","A#/Bb","B"};
const juce::StringArray Processor::scales {"Chromatic","Major","Minor","Major Pentatonic","Minor Pentatonic"};

Processor::Processor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "BLPAutoPitch", layout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor::layout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<AudioParameterFloat>(ParameterID{"tuning",1}, "Tuning",
        NormalisableRange<float>{0,100,0.01f}, 100.0f, AudioParameterFloatAttributes().withLabel("%")));
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"scale",1}, "Scale", scales, 0));
    p.add(std::make_unique<AudioParameterChoice>(ParameterID{"root",1}, "Root Note", keys, 0));
    p.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass",1}, "Bypass", false));
    return p;
}

bool Processor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto channels = l.getMainOutputChannelSet();
    return channels == l.getMainInputChannelSet()
        && (channels == juce::AudioChannelSet::mono() || channels == juce::AudioChannelSet::stereo());
}

void Processor::prepareToPlay(double sampleRate, int)
{
    fs = sampleRate;
    detector.prepare(fs);
    shifter.prepare(fs);
    setLatencySamples(shifter.latency);
    mix.reset(fs, 0.012);
    mix.setCurrentAndTargetValue(0);
    midi = period = target = shift = 0;
    missed = 2; lockedNote = -1;
}

void Processor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer&) { run(audio, false); }
void Processor::processBlockBypassed(juce::AudioBuffer<float>& audio, juce::MidiBuffer&) { run(audio, true); }
juce::AudioProcessorParameter* Processor::getBypassParameter() const { return params.getParameter("bypass"); }

void Processor::run(juce::AudioBuffer<float>& audio, bool forceDry)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = audio.getNumChannels();
    if (channels < 1 || channels > 2 || shifter.latency == 0) return;
    auto* const* data = audio.getArrayOfWritePointers();
    const auto amount = tuning->load() / 100.0;
    const auto selectedScale = int(scale->load()), selectedRoot = int(root->load());
    const auto smoothing = 1 - std::exp(-1 / (fs * (0.00055 + 0.090 * std::pow(1 - amount, 2))));
    const bool dry = forceDry || bypass->load() >= 0.5f || amount <= 0;
    if (selectedScale != lastScale || selectedRoot != lastRoot)
    {
        lockedNote = -1;
        lastScale = selectedScale;
        lastRoot = selectedRoot;
    }
    const auto updateTarget = [&]
    {
        if (missed >= 2) return;
        const int nearest = pitch::nearest(midi, selectedScale, selectedRoot);
        // Keep vibrato at the boundary from retriggering two adjacent notes.
        if (lockedNote < 0 || std::abs(midi - nearest) + 0.32 * amount < std::abs(midi - lockedNote))
            lockedNote = nearest;
        target = amount * (lockedNote - midi);
    };
    updateTarget();
    for (int i = 0; i < audio.getNumSamples(); ++i)
    {
        const float input = channels == 2 ? 0.5f * (data[0][i] + data[1][i]) : data[0][i];
        double hz;
        if (detector.push(input, hz))
        {
            if (hz > 0)
            {
                const auto detected = 69 + 12 * std::log2(hz / 440);
                midi = missed < 2 && std::abs(midi - detected) < 1.2
                    ? (0.24 * (1 - amount)) * midi + (0.76 + 0.24 * amount) * detected : detected;
                period = fs / (440 * std::exp2((midi - 69) / 12));
                missed = 0;
            }
            else if (missed < 2 && ++missed == 2) lockedNote = -1;
            updateTarget();
        }


        shift += smoothing * (target - shift);
        mix.setTargetValue(dry || missed >= 2 ? 0.0f : float(std::min(1.0, amount * 20.0)));
        const float wet = mix.getNextValue();
        shifter.process(data, channels, i, wet > 0 ? period : 0, std::exp2(shift / 12), wet);
    }
}

void Processor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = params.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void Processor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(params.state.getType())) params.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* Processor::createEditor() { return new Editor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor(); }








