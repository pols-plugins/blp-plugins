#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace spring
{
constexpr float pi = 3.14159265359f;

// First-order allpasses accumulate frequency-dependent travel time.
struct Dispersion
{
    float state = 0.0f;
    float tick(float x, float a) noexcept
    {
        const float y = state - a * x;
        state = x + a * y;
        return y;
    }
};

struct Tank
{
    static constexpr int count = 3;
    struct Line
    {
        std::vector<float> delay;
        std::array<Dispersion, 12> filters {};
        int write = 0;
        float damping = 0, dc = 0, phase = 0, rotation = 0;
        float length = 0, depth = 0, feedback = 0, initialPhase = 0;

        float read() noexcept
        {
            const int size = static_cast<int>(delay.size());
            float pos = static_cast<float>(write) - length - depth * std::sin(phase);
            if (pos < 0) pos += static_cast<float>(size);
            if (pos >= static_cast<float>(size)) pos = 0;
            const int base = static_cast<int>(pos);
            const int next = base + 1 == size ? 0 : base + 1;
            const float fraction = pos - static_cast<float>(base);
            phase += rotation;
            if (phase >= 2 * pi) phase -= 2 * pi;
            return delay[base] + fraction * (delay[next] - delay[base]);
        }

        void push(float x) noexcept
        {
            delay[write] = x;
            if (++write == static_cast<int>(delay.size())) write = 0;
        }
    };

    std::array<Line, count> lines;
    float low = 0, highpass = 0, cutoff = 0, loopDc = 0;
    bool british = false;

    void prepare(float rate, bool useUK, int channel)
    {
        british = useUK;
        low = 0;
        highpass = 1 - std::exp(-2 * pi * (british ? 105.0f : 185.0f) / rate);
        cutoff = 1 - std::exp(-2 * pi * (british ? 2600.0f : 6900.0f) / rate);
        loopDc = 1 - std::exp(-2 * pi * 65.0f / rate);
        constexpr std::array<float, count> american { 0.0239f, 0.0371f, 0.0517f };
        constexpr std::array<float, count> uk { 0.0313f, 0.0461f, 0.0649f };
        for (int i = 0; i < count; ++i)
        {
            auto& line = lines[i];
            line = {};
            const float time = (british ? uk : american)[i] * (1 + channel * (0.017f + i * 0.006f));
            line.length = rate * time;
            line.depth = rate * (british ? 0.000025f : 0.000058f) * (1 + 0.2f * i);
            line.delay.assign(static_cast<size_t>(std::ceil(line.length + line.depth)) + 4, 0);
            line.initialPhase = 1.7f * i + 0.8f * channel;
            line.rotation = 2 * pi * (0.73f + i * 0.49f) / rate;
            const float dispersionTime = (british ? 16.0f : 23.0f) / rate;
            line.feedback = std::pow(0.001f, (time + dispersionTime) / (british ? 1.78f : 2.35f));
        }
    }

    void clear() noexcept
    {
        low = 0;
        for (auto& line : lines)
        {
            std::fill(line.delay.begin(), line.delay.end(), 0.0f);
            line.filters = {};
            line.write = 0;
            line.damping = line.dc = 0;
            line.phase = line.initialPhase;
        }
    }

    float tick(float input) noexcept
    {
        low += highpass * (input - low);
        const float drive = input - low;
        std::array<float, count> reflections;
        for (int i = 0; i < count; ++i)
        {
            auto& line = lines[i];
            float reflected = line.read();
            for (int j = 0; j < static_cast<int>(line.filters.size()); ++j)
            {
                const float a = (british ? 0.29f : 0.51f) + 0.024f * j + 0.017f * i;
                reflected = line.filters[j].tick(reflected, a);
            }
            line.damping += cutoff * (reflected - line.damping);
            line.dc += loopDc * (line.damping - line.dc);
            reflections[i] = line.damping - line.dc;
        }

        // A Householder mix scatters the returns without adding loop gain.
        const float scatter = (2.0f / count) * (reflections[0] + reflections[1] + reflections[2]);
        for (int i = 0; i < count; ++i)
        {
            auto& line = lines[i];
            const float injection = i == 1 ? -0.37f : 0.40f;
            line.push(injection * drive + line.feedback * (reflections[i] - scatter));
        }
        return 0.94f * (reflections[0] - 0.89f * reflections[1] + 0.78f * reflections[2]);
    }
};
}

