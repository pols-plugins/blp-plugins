#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class VocalEnhancerDSP
{
public:
    void prepare(double sampleRate, float mix, int mode, bool bypass);
    void process(juce::AudioBuffer<float>&, float mix, int mode, bool bypass);

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Ramp = juce::SmoothedValue<float>;
    struct Voice
    {
        std::array<Filter, 4> eq;
        Filter sibilanceBand, harmonicLowpass;
        float previous = 0.0f, envelope = 0.0f;
    };

    struct Mode
    {
        std::array<Voice, 2> voices;
        Ramp weight;
    };

    std::array<Mode, 4> modes;
    Ramp mixRamp, activeRamp;
    float attack = 0.0f, release = 0.0f;
};
