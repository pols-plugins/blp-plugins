#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>

class DriverProcessor final : public juce::AudioProcessor
{
public:
    DriverProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BLP Driver"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override;

    struct Settings
    {
        float drive, tone, volume, bypass;
    };
    Settings readSettings() const noexcept;

    struct Signal
    {
        float low = 0.0f;

        float process(float dry, float amount, float inputGain, float coefficient, float outputGain) noexcept
        {
            const auto shaped = dry + amount * (std::tanh(dry * inputGain) - dry);
            low += coefficient * (shaped - low);
            return low * outputGain;
        }

        static float inputGain(float drive) noexcept { return 1.0f + 0.28f * drive * drive; }
        static float toneCoefficient(float hz, float rate) noexcept
        {
            return -std::expm1(-juce::MathConstants<float>::twoPi * juce::jmin(hz, 0.45f * rate) / rate);
        }
    };

    juce::AudioProcessorValueTreeState state;

private:
    void render(juce::AudioBuffer<float>&, bool);
    const std::atomic<float>* const driveParam;
    const std::atomic<float>* const toneParam;
    const std::atomic<float>* const volumeParam;
    const std::atomic<float>* const bypassParam;
    std::array<Signal, 2> channels{};
    juce::SmoothedValue<float> drive, toneCoefficient, volumeGain, wet;
    float rate = 44100.0f;
};
