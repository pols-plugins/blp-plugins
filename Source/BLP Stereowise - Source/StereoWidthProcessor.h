#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

class StereoWidthProcessor
{
public:
    void prepare(double sampleRate, float spread, float low, float high, bool bypass);
    void process(juce::AudioBuffer<float>& buffer, float spread, float low, float high, bool bypass);

private:
    juce::LinearSmoothedValue<float> lowAmount, highAmount;
    float previousMid = 0.0f, decorrelatedMid = 0.0f, allPass = 0.0f;
    float lowSide = 0.0f;
    float coefficient = 0.0f;
};

