#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

class ChorusProcessor final : public juce::AudioProcessor
{
public:
    ChorusProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BLP ST Chorus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.024; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override
    { return state.getParameter("bypass"); }
    static float rate(float normalized) { return 8.0f * normalized * normalized; }

    juce::AudioProcessorValueTreeState state;
private:
    std::atomic<float>* const speed;
    std::atomic<float>* const depth;
    std::atomic<float>* const bypass;
    juce::SmoothedValue<float> frequency, excursion, wet;
    juce::AudioBuffer<float> delay;
    int writeIndex = 0;
    double sampleRate = 44100.0, phase = 0.0;
};
