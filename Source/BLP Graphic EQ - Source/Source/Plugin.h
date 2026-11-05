#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "EqDSP.h"
class GraphicEQProcessor final : public juce::AudioProcessor {
public:
    GraphicEQProcessor();
    const juce::String getName() const override { return "BLP Graphic EQ"; }
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    double getTailLengthSeconds() const override { return 0.25; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    juce::AudioProcessorValueTreeState state;
    static constexpr const char* ids[] = {"band100","band200","band400","band800","band1600","band3200","band6400","output"};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void render(juce::AudioBuffer<float>&,bool);
    EqDSP dsp;
    std::array<std::atomic<float>*,8> raw{};
    std::atomic<float>* bypassValue{};
};
