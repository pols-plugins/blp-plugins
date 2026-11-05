#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>


class Compressor final : public juce::AudioProcessor
{
public:
    Compressor();
    inline static constexpr std::array names { "Attack", "Release", "Threshold", "Ratio", "Knee" };
    inline static constexpr std::array ids { "attack", "release", "threshold", "ratio", "knee", "bypass" };
    inline static constexpr std::array defaults { 18.f, 222.f, -17.5f, 12.1f, 6.f };
    juce::AudioProcessorValueTreeState state;

    const juce::String getName() const override { return "BLP DIGI Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
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

    std::array<std::atomic<float>*, 6> parameters {};
    std::array<juce::SmoothedValue<float>, 5> smooth;
    juce::SmoothedValue<float> wet;
    float reduction = 0.f;
    float timeScale = -1000.f / 48000.f;
};
