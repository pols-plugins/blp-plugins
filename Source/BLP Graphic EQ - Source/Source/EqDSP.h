#pragma once
#include <array>
#include <algorithm>
#include <cmath>

// RBJ constant-Q peaking EQs. All storage is fixed-size; no callback allocations.
class EqDSP {
public:
    static constexpr std::array<double, 7> frequencies {100,200,400,800,1600,3200,6400};
    using Gains = std::array<float, 7>;
    using Coeff = std::array<double, 5>; // b0,b1,b2,a1,a2, normalized by a0
    static Coeff coefficients(double rate, double frequency, double db) noexcept {
        constexpr double pi = 3.14159265358979323846;
        const auto w = 2*pi*std::min(frequency,rate*0.45)/rate;
        const auto a = std::pow(10.0, db/40.0);
        const auto alpha = std::sin(w)/(2*1.1);
        const auto c = std::cos(w), a0 = 1+alpha/a;
        return {(1+alpha*a)/a0,-2*c/a0,(1-alpha*a)/a0,-2*c/a0,(1-alpha/a)/a0};
    }
    void prepare(double rate, const Gains& initial, float output, bool bypass) noexcept {
        sr = std::isfinite(rate) && rate >= 8000 ? rate : 44100;
        smoothing = 1-std::exp(-32.0/(sr*0.025));
        gain = initial;
        for (size_t b=0;b<7;++b) coeff[b] = coefficients(sr,frequencies[b],gain[b]);
        delta = {}; memory = {}; countdown=0;
        out = std::pow(10.0, output/20.0); wet = bypass ? 0.0 : 1.0;
        sampleSmoothing = 1-std::exp(-1.0/(sr*0.010));
    }
    void process(float* const* data, int channels, int samples, const Gains& target, float output, bool bypass) noexcept {
        const double outTarget = std::pow(10.0, std::clamp(output,-15.0f,15.0f)/20.0);
        const double wetTarget = bypass ? 0.0 : 1.0;
        for(int n=0;n<samples;++n) {
            if(countdown==0) {
                for(size_t b=0;b<7;++b) {
                    const auto t = std::isfinite(target[b]) ? std::clamp(target[b],-15.0f,15.0f) : 0.0f;
                    gain[b] += float(smoothing*(t-gain[b]));
                    if(std::abs(t-gain[b])<0.00001f) gain[b]=t;
                    const auto next = coefficients(sr,frequencies[b],gain[b]);
                    for(size_t k=0;k<5;++k) delta[b][k]=(next[k]-coeff[b][k])/32.0;
                }
                countdown=32;
            }
            --countdown;
            for(size_t b=0;b<7;++b) for(size_t k=0;k<5;++k) coeff[b][k]+=delta[b][k];
            out += sampleSmoothing*(outTarget-out);
            wet += sampleSmoothing*(wetTarget-wet);
            if(std::abs(wetTarget-wet)<1e-9) wet=wetTarget;
            for(int ch=0;ch<std::min(channels,2);++ch) {
                const double dry = std::isfinite(data[ch][n]) ? data[ch][n] : 0.0;
                double x=dry;
                for(size_t b=0;b<7;++b) {
                    auto& s=memory[size_t(ch)][b]; const auto& c=coeff[b];
                    const double y=c[0]*x+s[0];
                    s[0]=c[1]*x-c[3]*y+s[1]; s[1]=c[2]*x-c[4]*y; x=y;
                }
                data[ch][n]=float(dry+wet*(out*x-dry));
            }
        }
    }
private:
    double sr=44100, smoothing=0.02, sampleSmoothing=0.002, out=1, wet=1;
    int countdown=0;
    Gains gain{};
    std::array<Coeff,7> coeff{},delta{};
    std::array<std::array<std::array<double,2>,7>,2> memory{};
};
