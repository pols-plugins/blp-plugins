#pragma once
#include "PitchShiftProcessor.h"

class PluginProcessor final : public juce::AudioProcessor
{
public:
    PluginProcessor();
    const juce::String getName() const override { return "BLP Pitch Shifter"; }
    void prepareToPlay(double rate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    juce::AudioProcessorParameter* getBypassParameter() const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState state;

private:
    void render(juce::AudioBuffer<float>&, bool hostBypass);
    PitchShiftProcessor shifter;
    std::atomic<float>* const mode = state.getRawParameterValue("input");
    std::atomic<float>* const shift = state.getRawParameterValue("shift");
    std::atomic<float>* const mix = state.getRawParameterValue("mix");
    std::atomic<float>* const bypass = state.getRawParameterValue("bypass");
};
