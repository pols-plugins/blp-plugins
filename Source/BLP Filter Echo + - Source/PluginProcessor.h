#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

class EchoProcessor final : public juce::AudioProcessor
{
public:
    EchoProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BLP Filter Echo +"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorValueTreeState state;
    static const juce::StringArray divisions;
private:
    void render(juce::AudioBuffer<float>&, bool);
    double delaySamples() const;
    enum Parameter { feedback, mode, division, milliseconds, cutoff, mix, bypass, count };
    std::array<std::atomic<float>*, count> values;
    float value(Parameter p) const { return values[p]->load(std::memory_order_relaxed); }
    juce::AudioBuffer<float> history;
    std::array<float, 2> filtered {};
    juce::SmoothedValue<float> feedbackGain, filterGain, wetGain;
    double rate = 44100.0, tempo = 120.0, from = 0.0, to = 0.0;
    int head = 0, fadeLength = 1, fadeRemaining = 0;
    bool needsInitialTap = true;
};
