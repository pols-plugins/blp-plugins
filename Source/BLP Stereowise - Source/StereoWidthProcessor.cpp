#include "StereoWidthProcessor.h"

void StereoWidthProcessor::prepare(double sampleRate, float spread, float low, float high, bool bypass)
{
    coefficient = static_cast<float>(-std::expm1(-juce::MathConstants<double>::twoPi * 500.0 / sampleRate));
    const auto tangent = std::tan(juce::MathConstants<double>::pi * 850.0 / sampleRate);
    allPass = static_cast<float>((tangent - 1.0) / (tangent + 1.0));
    previousMid = decorrelatedMid = lowSide = 0.0f;
    lowAmount.reset(sampleRate, 0.02);
    highAmount.reset(sampleRate, 0.02);
    const auto width = bypass ? 0.0f : spread * 0.1f;
    lowAmount.setCurrentAndTargetValue(width * low * 0.01f);
    highAmount.setCurrentAndTargetValue(width * high * 0.01f);
}

void StereoWidthProcessor::process(juce::AudioBuffer<float>& buffer, float spread, float low, float high, bool bypass)
{
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() != 2)
        return;

    const auto width = bypass ? 0.0f : spread * 0.1f;
    lowAmount.setTargetValue(width * low * 0.01f);
    highAmount.setTargetValue(width * high * 0.01f);
    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto side = (left[i] - right[i]) * 0.5f;
        const auto mid = (left[i] + right[i]) * 0.5f;
        decorrelatedMid = allPass * (mid - decorrelatedMid) + previousMid;
        previousMid = mid;
        // Reject DC and use a subtle phase-shifted copy for centered sources.
        const auto widthSource = side + 0.22f * (decorrelatedMid - mid);
        lowSide += coefficient * (widthSource - lowSide);
        const auto highSide = widthSource - lowSide;
        const auto lowGain = lowAmount.getNextValue();
        const auto highGain = highAmount.getNextValue();

        // Adding opposite deltas is equivalent to reconstructing from an unchanged mid.
        const auto delta = lowGain * lowSide + highGain * highSide;
        // Limit the added side to available headroom without clipping the dry signal.
        const auto ceiling = juce::jmax(1.0f, std::abs(left[i]), std::abs(right[i]));
        const auto safeDelta = juce::jlimit(juce::jmax(-ceiling - left[i], right[i] - ceiling),
                                            juce::jmin(ceiling - left[i], right[i] + ceiling), delta);
        left[i] += safeDelta;
        right[i] -= safeDelta;
    }
}

