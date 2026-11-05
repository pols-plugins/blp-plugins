#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace pitch
{
inline constexpr int scales[][12] = {
    {1,1,1,1,1,1,1,1,1,1,1,1}, {1,0,1,0,1,1,0,1,0,1,0,1},
    {1,0,1,1,0,1,0,1,1,0,1,0}, {1,0,1,0,1,0,0,1,0,1,0,0},
    {1,0,0,1,0,1,0,1,0,0,1,0}
};
inline int nearest(double midi, int scale, int root)
{
    const int base = int(std::floor(midi));
    int best = base;
    double distance = 100.0;
    for (int n = base - 3; n <= base + 4; ++n)
        if (scales[scale][(n - root + 120) % 12] && std::abs(n - midi) < distance)
        {
            best = n;
            distance = std::abs(n - midi);
        }
    return best;
}

class Detector
{
    static constexpr int window = 192, maxLag = 200, size = 512;
    std::array<float, size> samples {};
    std::array<float, maxLag + 1> difference {};
    int index = 0, count = 0, subdivision = 0, decimation = 1;
    float sum = 0;
    double rate = 12000;
public:
    void prepare(double fs)
    {
        *this = {};
        decimation = std::max(1, int(std::round(fs / 12000)));
        rate = fs / decimation;
    }

    bool push(float x, double& frequency)
    {
        sum += x;
        if (++subdivision < decimation) return false;
        samples[index++ & (size - 1)] = sum / float(decimation);
        index &= size - 1;
        sum = 0;
        subdivision = 0;
        if (++count < 64) return false;
        count = 0;
        frequency = estimate();
        return true;
    }

private:
    double estimate()
    {
        const int lastLag = std::min(maxLag, int(rate / 65));
        const int firstLag = std::max(2, int(rate / 1200));
        const int start = index - window;
        float energy = 0;
        for (int i = 0; i < window; ++i)
        {
            const float x = samples[(start + i) & (size - 1)];
            energy += x * x;
        }
        if (energy < window * 1.0e-7f) return 0;
        float total = 0;
        difference[0] = 1;
        for (int lag = 1; lag <= lastLag; ++lag)
        {
            float value = 0;
            for (int i = 0; i < window; ++i)
            {
                const float delta = samples[(start + i) & (size - 1)]
                                  - samples[(start + i - lag) & (size - 1)];
                value += delta * delta;
            }
            total += value;
            difference[lag] = total > 0 ? value * float(lag) / total : 1.0f;
        }
        for (int lag = firstLag; lag < lastLag; ++lag)
        {
            if (difference[lag] >= 0.17f) continue;
            while (lag < lastLag && difference[lag + 1] < difference[lag]) ++lag;
            if (lag == lastLag) return 0;
            const double left = difference[lag - 1], mid = difference[lag], right = difference[lag + 1];
            const double curvature = left + right - 2 * mid;
            return rate / (lag + (curvature > 0 ? 0.5 * (left - right) / curvature : 0));
        }
        return 0;
    }
};

// Pitch-synchronous overlap/add. Grains read the source at unity speed.
// Changing the spacing between grains changes pitch while the source timeline advances normally.
class Shifter
{
    struct Grain { double start = 0, length = 0; double age = 0, onset = 0; };
    std::array<Grain, 32> grains {};
    std::array<std::vector<float>, 2> history;
    int mask = 0, lead = 0;
    std::int64_t position = 0;
    double countdown = 0, previousCenter = 0;
    bool anchored = false;
public:
    int latency = 0;
    void prepare(double fs)
    {
        latency = int(std::round(fs * 0.006));
        lead = int(std::ceil(fs * 0.005));
        int size = 1;
        while (size < fs * 0.072) size *= 2;
        for (auto& channel : history) channel.assign(size, 0);
        mask = size - 1;
        position = 0;
        grains = {};
        countdown = 0;
        anchored = false;
    }

    void process(float* const* channels, int count, int sample, double period, double ratio, float mix)
    {
        for (int c = 0; c < count; ++c)
            history[c][position & mask] = channels[c][sample];
        if (period > 0 && --countdown <= 0)
        {
            const double desired = double(position + lead - latency) + countdown;
            double center = desired;
            if (anchored)
            {
                center = previousCenter + period * std::round((desired - previousCenter) / period);
                // Read only recorded audio. An earlier pitch cycle has the same phase.
                const double latest = double(position + lead) + countdown - 2;
                if (center + 0.25 * period > latest) center -= period;
                center = alignCenter(center, period, count);
            }
            previousCenter = center;
            anchored = true;
            for (auto& grain : grains)
                if (grain.length == 0)
                {
                    const double age = period - lead - countdown;
                    grain = {center - period, 2 * period, age, age};
                    break;
                }
            countdown += period / ratio;
        }
        if (period <= 0)
        {
            anchored = false;
            countdown = 0;
        }
        float wet[2] {};
        float weight = 0;
        for (auto& grain : grains)
        {
            if (grain.length == 0) continue;
            if (grain.age < 0)
            {
                ++grain.age;
                continue;
            }
            const float phase = float(grain.age / grain.length);
            // Window with zero amplitude and slope at both endpoints.
            const float fade = grain.onset > 0
                ? float(std::min(1.0, (grain.age - grain.onset) / (0.33 * lead))) : 1.0f;
            const float w = fade * fade * (3 - 2 * fade)
                * 16 * phase * phase * (1 - phase) * (1 - phase);
            const double source = grain.start + grain.age;
            const auto i = std::int64_t(std::floor(source));
            const float fraction = float(source - double(i));
            for (int c = 0; c < count; ++c)
            {
                const auto& buffer = history[c];
                const float a = buffer[i & mask], b = buffer[(i + 1) & mask];
                wet[c] += w * (a + fraction * (b - a));
            }
            weight += w;
            if (++grain.age >= grain.length) grain.length = 0;
        }
        for (int c = 0; c < count; ++c)
        {
            const auto dry = history[c][(position - latency) & mask];
            const float coverage = std::min(1.0f, weight * 4.0f);
            channels[c][sample] = weight > 0 ? dry + mix * coverage * (wet[c] / weight - dry) : dry;
        }
        ++position;
    }

private:
    float reference(double time, int count) const
    {
        const auto i = std::int64_t(std::floor(time));
        const float fraction = float(time - double(i));
        const auto sample = [&](int c)
        {
            const auto& h = history[c];
            return h[i & mask] + fraction * (h[(i + 1) & mask] - h[i & mask]);
        };
        return count == 2 ? 0.5f * (sample(0) + sample(1)) : sample(0);
    }

    double alignCenter(double predicted, double period, int count) const
    {
        if (std::abs(predicted - previousCenter) < 1.0) return previousCenter;

        // Match the phase of the previous grain on the available history.
        // Search locally so modulation cannot pull the source off its timeline.
        // Keep every correlation tap in the recorded history, including for low notes.
        const double lookback = std::max(0.0, std::max(predicted + period * 0.25, previousCenter)
                                               - double(position - 1));
        std::array<float, 16> templateSamples {};
        double templatePower = 0;
        for (int tap = 0; tap < 16; ++tap)
        {
            const double offset = (tap - 15) * period / 24 - lookback;
            const auto v = reference(previousCenter + offset, count);
            templateSamples[tap] = v;
            templatePower += v * v;
        }
        if (templatePower < 1.0e-8) return predicted;

        double best = -2, center = predicted;
        const auto search = [&](double around, double step, int radius)
        {
            for (int j = -radius; j <= radius; ++j)
            {
                const double candidate = around + j * step;
                double cross = 0, power = 0;
                for (int tap = 0; tap < 16; ++tap)
                {
                    const auto v = reference(candidate + (tap - 15) * period / 24 - lookback, count);
                    cross += v * templateSamples[tap];
                    power += v * v;
                }
                if (power < 1.0e-8) continue;
                const double penalty = 0.12 * std::abs(candidate - predicted) / period;
                const double score = cross / std::sqrt(power * templatePower) - penalty;
                if (score > best)
                {
                    best = score;
                    center = candidate;
                }
            }
        };
        search(predicted, period / 32, 7);
        search(center, period / 128, 3);
        return best > 0.55 ? center : predicted;
    }
};
}








