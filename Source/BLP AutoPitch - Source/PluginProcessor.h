#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PitchEngine.h"
class Processor final : public juce::AudioProcessor
{
public:
    Processor();
    static const juce::StringArray keys, scales;
    juce::AudioProcessorValueTreeState params;

    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BLP AutoPitch"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return double(shifter.latency) / fs; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void run(juce::AudioBuffer<float>&, bool);

    std::atomic<float>* const tuning = params.getRawParameterValue("tuning");
    std::atomic<float>* const scale = params.getRawParameterValue("scale");
    std::atomic<float>* const root = params.getRawParameterValue("root");
    std::atomic<float>* const bypass = params.getRawParameterValue("bypass");
    pitch::Detector detector;
    pitch::Shifter shifter;
    juce::SmoothedValue<float> mix;
    double fs = 48000, midi = 0, period = 0, shift = 0, target = 0;
    int missed = 2, lockedNote = -1, lastScale = -1, lastRoot = -1;
};





