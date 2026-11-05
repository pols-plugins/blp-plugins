#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

class Chorus final : public juce::AudioProcessor
{
public:
    Chorus();
    inline static constexpr std::array names { "Depth", "Amount", "Speed", "Bass", "Treble" };
    inline static constexpr std::array ids { "depth", "amount", "speed", "bass", "treble", "bypass" };
    inline static constexpr std::array defaults { 4.f, 5.f, 2.2f, 5.f, 5.f };
    juce::AudioProcessorValueTreeState state;

    const juce::String getName() const override { return "BLP Chorus Ensemble"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.3; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter(ids[5]); }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

private:
    void render(juce::AudioBuffer<float>&, bool);
    void updateTargets(bool);
    std::array<std::atomic<float>*, 6> parameters {};
    std::array<juce::SmoothedValue<float>, 5> smooth;
    std::array<std::vector<float>, 2> delay;
    std::array<float, 2> bassLow {}, trebleLow {};
    int position = 0;
    float rate = 48000.f;
    double phase = 0.0;
};
