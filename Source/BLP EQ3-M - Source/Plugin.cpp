#include "BlpEditorTheme.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
using namespace juce;

namespace {
constexpr const char* ids[]{"lowFreq","lowGain","midFreq","midGain","midQ","highFreq","highGain","output","bypass"};
class EQ final : public AudioProcessor {
    struct Biquad {
        std::array<double,6> c{1,0,0,1,0,0};
        double z1=0,z2=0;
        double tick(double x) {
            const auto y=c[0]*x+z1;
            z1=c[1]*x-c[4]*y+z2;
            z2=c[2]*x-c[5]*y;
            return y;
        }
    };
    std::array<std::array<Biquad,3>,2> filters;
    std::array<SmoothedValue<double>,9> smooth;
    double rate=44100;
    unsigned phase=0;

    static AudioProcessorValueTreeState::ParameterLayout layout() {
        AudioProcessorValueTreeState::ParameterLayout result;
        const float low[]{40,-30,100,-30,.3f,1000,-30,-30};
        const float high[]{500,30,8000,30,4,16000,30,12};
        const float initial[]{120,0,8000,0,1,8000,0,0};
        for(int i=0;i<8;++i) {
            NormalisableRange<float> range(low[i],high[i],i==1||i==3||i==6?1.f:0.f);
            if(i==0||i==2||i==5)
                range={low[i],high[i],
                    [](float a,float b,float p){return a*std::pow(b/a,p);},
                    [](float a,float b,float v){return std::log(v/a)/std::log(b/a);},
                    [i](float a,float b,float v){return jlimit(a,b,i==2?std::round(v):v);}};
            auto format=AudioParameterFloatAttributes{};
            if(i==2) format=format.withStringFromValueFunction([](float v,int){return String(roundToInt(v));});
            result.add(std::make_unique<AudioParameterFloat>(ParameterID(ids[i],1),ids[i],range,initial[i],format));
        }
        result.add(std::make_unique<AudioParameterBool>(ParameterID("bypass",1),"Bypass",false));
        return result;
    }

    void updateCoefficients() {
        using C=dsp::IIR::ArrayCoefficients<double>;
        auto freq=[&](int i){return jlimit(10.,rate*.45,smooth[i].getCurrentValue());};
        auto gain=[&](int i){return Decibels::decibelsToGain(smooth[i].getCurrentValue());};
        std::array<std::array<double,6>,3> coefficients{
            C::makeLowShelf(rate,freq(0),.70710678,gain(1)),
            C::makePeakFilter(rate,freq(2),smooth[4].getCurrentValue(),gain(3)),
            C::makeHighShelf(rate,freq(5),.70710678,gain(6))};
        for(auto& c:coefficients) { const auto a0=c[3]; for(auto& value:c) value/=a0; }
        for(auto& channel:filters)
            for(size_t band=0;band<channel.size();++band) channel[band].c=coefficients[band];
    }
    void process(AudioBuffer<float>& audio,bool bypassed) {
        ScopedNoDenormals guard;
        for(size_t i=0;i<smooth.size();++i)
            smooth[i].setTargetValue(i==8&&bypassed?1.0:state.getRawParameterValue(ids[i])->load());
        for(int n=0;n<audio.getNumSamples();++n) {
            for(auto& value:smooth) value.getNextValue();
            if(phase++%8==0) updateCoefficients();
            const auto gain=Decibels::decibelsToGain(smooth[7].getCurrentValue());
            const auto dryMix=smooth[8].getCurrentValue();
            for(int ch=0;ch<jmin(2,audio.getNumChannels());++ch) {
                const double dry=audio.getSample(ch,n);
                double wet=dry;
                for(auto& filter:filters[(size_t)ch]) wet=filter.tick(wet);
                audio.setSample(ch,n,(float)(dryMix*dry+(1-dryMix)*wet*gain));
            }
        }
    }
public:
    AudioProcessorValueTreeState state{*this,nullptr,"EQ3M",layout()};
    EQ():AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true)
                                         .withOutput("Output",AudioChannelSet::stereo(),true)) {}
    const String getName() const override {return "BLP EQ3-M";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return .2;}
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override {return {};}
    void changeProgramName(int,const String&) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override {
        auto output=l.getMainOutputChannelSet();
        return l.getMainInputChannelSet()==output&&(output==AudioChannelSet::mono()||output==AudioChannelSet::stereo());
    }
    AudioProcessorParameter* getBypassParameter() const override {return state.getParameter("bypass");}
    void prepareToPlay(double sampleRate,int) override {
        rate=sampleRate; phase=0; filters={};
        for(size_t i=0;i<smooth.size();++i) {
            smooth[i].reset(rate,.03);
            smooth[i].setCurrentAndTargetValue(state.getRawParameterValue(ids[i])->load());
        }
        updateCoefficients();
    }
    void processBlock(AudioBuffer<float>& b,MidiBuffer&) override {process(b,false);}
    void processBlockBypassed(AudioBuffer<float>& b,MidiBuffer&) override {process(b,true);}
    void getStateInformation(MemoryBlock& b) override {copyXmlToBinary(*state.copyState().createXml(),b);}
    void setStateInformation(const void* data,int size) override {
        if(auto xml=getXmlFromBinary(data,size);xml&&xml->hasTagName("EQ3M")) state.replaceState(ValueTree::fromXml(*xml));
    }
    bool hasEditor() const override {return true;}
    AudioProcessorEditor* createEditor() override;
};

class Style final : public blp::EditorLookAndFeel {
    void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float p,float start,float end,Slider&) override {
        const auto cx=x+w*.5f,cy=y+h*.5f,r=jmin(w,h)*.5f-7;
        g.setColour(Colour(0xff323238));
        g.drawEllipse(cx-r,cy-r,2*r,2*r,2);
        const auto angle = start + p * (end - start);
        if (p > 0.0f)
        {
            Path accentArc;
            accentArc.addCentredArc(cx, cy, r, r, 0.0f, start, angle, true);
            g.setColour(blp::accent);
            g.strokePath(accentArc, PathStrokeType(2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour(Colours::white);
        g.fillEllipse(cx-r+5,cy-r+5,2*r-10,2*r-10);
        g.setColour(Colour(0xff151518));
        g.drawLine(cx,cy,cx+std::sin(angle)*(r-8),cy-std::cos(angle)*(r-8),2);
    }
};

class Editor final : public blp::ThemedEditor {
    Style style;
    std::array<Slider,4> knobs;
    std::array<Label,4> labels;
    std::array<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>,4> attachments;
    blp::BypassButton bypass;
    AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
public:
    explicit Editor(EQ& eq):blp::ThemedEditor(eq),bypassAttachment(eq.state,"bypass",bypass) {
        setLookAndFeel(&style);
        setSize(300, 270);
        constexpr int params[]{1,3,6,2}, x[]{24,114,204,114}, y[]{54,54,54,146};
        constexpr const char* names[]{"Low","Mid","High","Mid Sweep"};
        for(int i=0;i<4;++i) {
            auto& knob=knobs[(size_t)i];
            addAndMakeVisible(knob);
            knob.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            knob.setTextBoxStyle(i==3?Slider::TextBoxBelow:Slider::NoTextBox,false,72,19);
            knob.setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);
            knob.setBounds(x[i],y[i],72,77);
            const auto* id=ids[params[i]];
            auto* param=eq.state.getParameter(id);
            knob.setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));
            attachments[(size_t)i]=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(eq.state,id,knob);
            if(i==3) {
                knob.textFromValueFunction=[](double value){return String(roundToInt(value));};
                knob.updateText();
                blp::showValueOnHover(knob, *this);
            } else {
                knob.textFromValueFunction=[](double value){return String(value,1)+" dB";};
                knob.setPopupDisplayEnabled(false,true,this);
            }
            auto& label=labels[(size_t)i];
            label.setText(names[i],dontSendNotification);
            label.setJustificationType(Justification::centred);
            label.setColour(Label::textColourId,blp::textWhite);
            label.setFont(FontOptions(12));
            label.setBounds(x[i]-5,y[i]+(i==3 ? 76 : 70),82,18);
            addAndMakeVisible(label);
        }
        addAndMakeVisible(bypass);
        bypass.setBounds(blp::bypassBounds(getWidth()));
    }
    ~Editor() override {setLookAndFeel(nullptr);}
    void paint(Graphics& g) override {
        g.fillAll(blp::canvas);
        g.setColour(Colours::white);
        g.setFont(14);
        
        blp::paintHeader(g, getWidth(), "BLP EQ3-M");
}
};
AudioProcessorEditor* EQ::createEditor() {return new Editor(*this);}
}
AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new EQ;}
