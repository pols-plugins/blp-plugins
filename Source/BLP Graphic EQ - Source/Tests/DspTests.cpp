#include "../Source/EqDSP.h"
#include <iostream>
#include <vector>
#include <random>
#include <stdexcept>
#include <complex>
#include <chrono>
void check(bool ok,const char* msg) { if(!ok) throw std::runtime_error(msg); }
double measure(double sr,double f,EqDSP::Gains gains,float out=0,bool bypass=false) {
    EqDSP dsp; dsp.prepare(sr,gains,out,bypass);
    std::array<float,257> left{},right{}; float* ptr[]={left.data(),right.data()};
    double input=0,output=0; int offset=0;
    for(int block=0;block<600;++block) {
        for(int n=0;n<257;++n) { left[n]=float(0.1*std::sin(6.283185307179586*f*(offset+n)/sr)); right[n]=0; }
        auto dry=left;
        dsp.process(ptr,2,257,gains,out,bypass);
        for(int n=0;n<257;++n) {
            check(std::isfinite(left[n]),"non-finite output");
            check(right[n]==0,"stereo crosstalk");
            if(block>300) { input+=dry[n]*dry[n]; output+=left[n]*left[n]; }
        }
        offset+=257;
    }
    return 10*std::log10(output/input);
}
int main() {
    try {
        for(double sr:{44100.,48000.,88200.,96000.}) {
            check(std::abs(measure(sr,997,{}))<0.0001,"flat response");
            for(size_t b=0;b<7;++b) for(float db:{-15.0f,15.0f}) {
                EqDSP::Gains g{}; g[b]=db;
                check(std::abs(measure(sr,EqDSP::frequencies[b],g)-db)<0.03,"band centre gain");
                double far=b<3?18000:30;
                check(std::abs(measure(sr,far,g))<0.15,"band isolation");
                check(std::abs(measure(sr,997,g,15,true))<0.0001,"bypass unity");
            }
            for(float db:{-15.f,15.f}) check(std::abs(measure(sr,997,{},db)-db)<0.002,"output gain");
            EqDSP d; d.prepare(sr,{},0,false);
            std::array<float,127> l{},r{}; float* data[]={l.data(),r.data()};
            std::mt19937 rng(42); std::uniform_real_distribution<float> dist(-1,1);
            for(int block=0;block<8000;++block) {
                EqDSP::Gains g; for(auto& v:g) v=15*dist(rng);
                for(size_t n=0;n<l.size();++n) l[n]=r[n]=0.001f*dist(rng);
                d.process(data,2,127,g,15*dist(rng),block%3==0);
                for(size_t n=0;n<l.size();++n) {
                    check(std::isfinite(l[n])&&std::abs(l[n])<10,"automation stability");
                    check(l[n]==r[n],"stereo consistency");
                }
            }
            std::cout<<"PASS "<<sr<<" Hz: flat, 7 +/-15 dB bands, isolation, +/-15 dB output, bypass, stereo, automation stress\n";
        }
        // Constant DC makes control discontinuities visible without a source waveform hiding them.
        EqDSP smooth; smooth.prepare(48000,{},0,false);
        float sample=0.01f; float* ptr[]={&sample}; float previous=sample,largestStep=0;
        EqDSP::Gains extreme{}; extreme.fill(15);
        for(int n=0;n<96000;++n) {
            sample=0.01f;
            smooth.process(ptr,1,1,n<1000?EqDSP::Gains{}:extreme,n<1000?0:15,n>48000);
            largestStep=std::max(largestStep,std::abs(sample-previous)); previous=sample;
        }
        check(largestStep<0.001f,"abrupt control discontinuity");
        std::cout<<"PASS smoothed gain/output/bypass transitions; largest DC step "<<largestStep<<"\n";
        EqDSP perf; perf.prepare(48000,extreme,0,false);
        std::array<float,256> l{},r{}; float* stereo[]={l.data(),r.data()};
        const auto start=std::chrono::steady_clock::now();
        for(int block=0;block<1875;++block) {
            l.fill(0.01f);r.fill(0.01f);perf.process(stereo,2,256,extreme,0,false);
        }
        auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"Benchmark: 10 seconds of stereo 48 kHz audio processed in "<<seconds<<" seconds ("<<seconds*10<<"% of one core in this test)\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
