#include "VocalEnhancerDSP.h"
#include <cmath>

namespace
{
struct Tuning
{
    float bodyHz, bodyDb, cutHz, cutDb, presenceHz, presenceDb, airHz, airDb;
    float harmonic, trimDb;
    bool bodyShelf;
};

constexpr std::array<Tuning, 4> tuning {{
    { 220, 2.1f, 470, -1.4f, 2800, 1.0f, 9000, -0.9f, 0.090f, -0.45f, false },
    { 180, 0.0f, 340, -1.5f, 3100, 1.6f, 9600,  1.3f, 0.022f, -0.35f, false },
    { 145, 2.0f, 390, -2.1f, 4100, 2.4f, 8800,  2.9f, 0.075f, -0.80f, false },
    { 170, 1.7f, 420, -1.1f, 3200, 0.8f, 8200,  2.1f, 0.015f, -0.55f, true }
}};

float gain(float db) { return juce::Decibels::decibelsToGain(db); }

float protectPeaks(float wet, float dry)
{
    // Smooth knee with unity slope at onset. Allow an already-overrange
    // input to pass without adding further peak gain.
    const auto ceiling = juce::jmax(1.0f, std::abs(dry));
    const auto knee = ceiling * 0.92f;
    const auto magnitude = std::abs(wet);
    if (magnitude <= knee)
        return wet;
    const auto width = ceiling - knee;
    return std::copysign(knee + width * std::tanh((magnitude - knee) / width), wet);
}
}

void VocalEnhancerDSP::prepare(double sampleRate, float mix, int mode, bool bypass)
{
    using Coeff = juce::dsp::IIR::Coefficients<float>;
    attack = std::exp(-1.0f / static_cast<float>(sampleRate * 0.002));
    release = std::exp(-1.0f / static_cast<float>(sampleRate * 0.035));
    mixRamp.reset(sampleRate, 0.020);
    activeRamp.reset(sampleRate, 0.012);
    mixRamp.setCurrentAndTargetValue(mix);
    activeRamp.setCurrentAndTargetValue(bypass ? 0.0f : 1.0f);

    for (size_t index = 0; index < modes.size(); ++index)
    {
        auto& path = modes[index];
        const auto& t = tuning[index];
        path.weight.reset(sampleRate, 0.025);
        path.weight.setCurrentAndTargetValue(index == static_cast<size_t>(mode) ? 1.0f : 0.0f);
        for (auto& voice : path.voices)
        {
            voice.eq[0].coefficients = t.bodyShelf
                ? Coeff::makeLowShelf(sampleRate, t.bodyHz, 0.71f, gain(t.bodyDb))
                : Coeff::makePeakFilter(sampleRate, t.bodyHz, 0.80f, gain(t.bodyDb));
            voice.eq[1].coefficients = Coeff::makePeakFilter(sampleRate, t.cutHz, 1.0f, gain(t.cutDb));
            voice.eq[2].coefficients = Coeff::makePeakFilter(sampleRate, t.presenceHz, 0.85f, gain(t.presenceDb));
            voice.eq[3].coefficients = Coeff::makeHighShelf(sampleRate, t.airHz, 0.71f, gain(t.airDb));
            for (auto& filter : voice.eq)
                filter.reset();
            voice.sibilanceBand.coefficients = Coeff::makeBandPass(sampleRate, 7200.0f, 1.15f);
            voice.sibilanceBand.reset();
            voice.harmonicLowpass.coefficients = Coeff::makeLowPass(sampleRate, 9000.0f);
            voice.harmonicLowpass.reset();
            voice.previous = voice.envelope = 0.0f;
        }
    }
}

void VocalEnhancerDSP::process(juce::AudioBuffer<float>& buffer, float mix, int mode, bool bypass)
{
    juce::ScopedNoDenormals noDenormals;
    mixRamp.setTargetValue(mix);
    activeRamp.setTargetValue(bypass ? 0.0f : 1.0f);
    std::array<float, 4> trims;
    for (size_t index = 0; index < modes.size(); ++index)
    {
        modes[index].weight.setTargetValue(index == static_cast<size_t>(mode) ? 1.0f : 0.0f);
        trims[index] = gain(tuning[index].trimDb + 0.5f);
    }

    auto channels = buffer.getArrayOfWritePointers();
    const auto count = juce::jmin(2, buffer.getNumChannels());
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto amount = mixRamp.getNextValue() * activeRamp.getNextValue();
        std::array<float, 2> enhanced {};
        for (size_t index = 0; index < modes.size(); ++index)
        {
            auto& path = modes[index];
            const auto weight = path.weight.getNextValue();
            std::array<float, 2> wet {}, sibilance {};
            float linkedEnvelope = 0.0f;
            for (int channel = 0; channel < count; ++channel)
            {
                auto& voice = path.voices[static_cast<size_t>(channel)];
                auto x = channels[channel][sample];
                for (auto& filter : voice.eq)
                    x = filter.processSample(x);

                // Average two samples of the nonlinear residue and filter
                // that branch alone to reduce aliasing without delaying dry audio.
                const auto midpoint = (x + voice.previous) * 0.5f;
                voice.previous = x;
                const auto residue = 0.5f * (std::tanh(x) - x + std::tanh(midpoint) - midpoint);
                x += tuning[index].harmonic * voice.harmonicLowpass.processSample(residue);
                if (index == 2)
                {
                    sibilance[channel] = voice.sibilanceBand.processSample(x);
                    const auto level = std::abs(sibilance[channel]);
                    const auto coefficient = level > voice.envelope ? attack : release;
                    voice.envelope = level + coefficient * (voice.envelope - level);
                    linkedEnvelope = juce::jmax(linkedEnvelope, voice.envelope);
                }
                wet[channel] = x;
            }
            const auto reduction = juce::jlimit(0.0f, 0.35f, (linkedEnvelope - 0.055f) * 2.0f);
            for (int channel = 0; channel < count; ++channel)
                enhanced[channel] += weight * trims[index] * (wet[channel] - reduction * sibilance[channel]);
        }

        if (amount > 0.0f)
            for (int channel = 0; channel < count; ++channel)
            {
                auto& dry = channels[channel][sample];
                dry += amount * (protectPeaks(enhanced[channel], dry) - dry);
            }
    }
}
