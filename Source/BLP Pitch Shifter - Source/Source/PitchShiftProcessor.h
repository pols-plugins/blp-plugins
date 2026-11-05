#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <signalsmith-stretch/signalsmith-stretch.h>
#include <array>
#include <vector>
#include <cmath>

class PitchShiftProcessor
{
public:
    int prepare(double rate, int channelCount, float shift, float mix, int mode, bool bypass)
    {
        channels = channelCount;

        constexpr double windows[] { 0.10, 0.075, 0.040 };
        constexpr double hops[] { 0.025, 0.01875, 0.008 };
        latency = 0;
        for (int i = 0; i < 3; ++i)
        {
            engines[i].configure(channels, int(rate * windows[i]), int(rate * hops[i]));
            delays[i] = engines[i].inputLatency() + engines[i].outputLatency();
            latency = juce::jmax(latency, delays[i]);
            weights[i].reset(rate, 0.04);
            weights[i].setCurrentAndTargetValue(i == mode ? 1.0f : 0.0f);
        }
        engines[0].setFormantFactor(1, true);
        engines[0].setFormantBase(float(180 / rate));
        for (auto& line : history)
            for (auto& channel : line)
                channel.assign(size_t(latency + 1), 0.0f);
        cursor = 0;
        pitch.reset(rate, 0.04);
        wetMix.reset(rate, 0.015);
        pitch.setCurrentAndTargetValue(shift);
        wetMix.setCurrentAndTargetValue(bypass ? 0.0f : mix);
        return latency;
    }

    void process(juce::AudioBuffer<float>& buffer, float shift, float mix, int mode, bool bypass)
    {
        pitch.setTargetValue(shift);
        wetMix.setTargetValue(bypass ? 0.0f : mix);
        for (int i = 0; i < 3; ++i)
            weights[i].setTargetValue(i == mode ? 1.0f : 0.0f);

        for (int start = 0; start < buffer.getNumSamples(); start += chunk)
        {
            const int count = juce::jmin(chunk, buffer.getNumSamples() - start);
            std::array<const float*, 2> input {};
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* samples = buffer.getWritePointer(ch, start);
                for (int n = 0; n < count; ++n)
                    if (!std::isfinite(samples[n]))
                        samples[n] = 0.0f;
                input[ch] = samples;
            }

            const float ratio = std::exp2(pitch.skip(count) / 12.0f);
            for (int i = 0; i < 3; ++i)
            {
                auto& engine = engines[i];
                engine.setTransposeFactor(ratio);
                std::array<float*, 2> output { wet[i][0].data(), wet[i][1].data() };
                engine.process(input, count, output, count);
            }

            for (int n = 0; n < count; ++n)
            {
                const float amount = wetMix.getNextValue();
                std::array<float, 3> blend {};
                for (int i = 0; i < 3; ++i)
                    blend[i] = weights[i].getNextValue();
                for (int ch = 0; ch < channels; ++ch)
                {
                    history[3][ch][cursor] = input[ch][n];
                    const float dry = read(3, ch, latency);
                    float shifted = 0.0f;
                    for (int i = 0; i < 3; ++i)
                    {
                        const float value = wet[i][ch][n];
                        history[i][ch][cursor] = std::isfinite(value) ? value : 0.0f;
                        shifted += blend[i] * read(i, ch, latency - delays[i]);
                    }
                    buffer.setSample(ch, start + n, amount == 0.0f ? dry :
                        juce::jlimit(-1.0f, 1.0f, dry + amount * (shifted - dry)));
                }
                if (++cursor > latency)
                    cursor = 0;
            }
        }
    }

private:
    float read(int line, int channel, int delay) const
    {
        return history[line][channel][(cursor + latency + 1 - delay) % (latency + 1)];
    }

    static constexpr int chunk = 64;
    int channels = 0, latency = 0, cursor = 0;

    std::array<signalsmith::stretch::SignalsmithStretch<float>, 3> engines;
    std::array<int, 3> delays {};
    std::array<std::array<std::array<float, chunk>, 2>, 3> wet {};
    std::array<std::array<std::vector<float>, 2>, 4> history;
    std::array<juce::SmoothedValue<float>, 3> weights;
    juce::SmoothedValue<float> pitch, wetMix;
};






