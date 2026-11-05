#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

class BitcrusherProcessor final : public juce::AudioProcessor
{
public:
    BitcrusherProcessor();

    void prepareToPlay(double sampleRate, int maxBlockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

    const juce::String getName() const override { return "BLP Bitcrusher"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    void render(juce::AudioBuffer<float>&, bool hostBypass);

    std::atomic<float>* const intensityValue;
    std::atomic<float>* const bitsValue;
    std::atomic<float>* const mixValue;
    std::atomic<float>* const bypassValue;
    juce::SmoothedValue<float> intensity, bits, mix, active;
    std::array<float, 2> held {};
    float phase = 0.0f;
};
