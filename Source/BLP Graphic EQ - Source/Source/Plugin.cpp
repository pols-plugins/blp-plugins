#include "BlpEditorTheme.h"
#include "Plugin.h"

juce::AudioProcessorValueTreeState::ParameterLayout GraphicEQProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    const char* names[]={"100 Hz","200 Hz","400 Hz","800 Hz","1.6 kHz","3.2 kHz","6.4 kHz","Output"};
    for(int i=0;i<8;++i) {
        const float limit=15.0f;
        p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID(ids[i],1),names[i],
            juce::NormalisableRange<float>(-limit,limit,0.1f),0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    }
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("bypass",1),"Bypass",false));
    return p;
}
GraphicEQProcessor::GraphicEQProcessor()
 : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
   state(*this,nullptr,"BLPGraphicEQState",layout()) {
    for(size_t i=0;i<8;++i) raw[i]=state.getRawParameterValue(ids[i]);
    bypassValue=state.getRawParameterValue("bypass");
}
bool GraphicEQProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    return l.getMainInputChannelSet()==l.getMainOutputChannelSet()
      && (l.getMainOutputChannelSet()==juce::AudioChannelSet::mono() || l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());
}
void GraphicEQProcessor::prepareToPlay(double sr,int) {
    EqDSP::Gains g; for(size_t i=0;i<7;++i) g[i]=raw[i]->load();
    dsp.prepare(sr,g,raw[7]->load(),bypassValue->load()>0.5f);
    setLatencySamples(0);
}
void GraphicEQProcessor::render(juce::AudioBuffer<float>& b,bool bypass) {
    juce::ScopedNoDenormals noDenormals;
    for(int c=getTotalNumInputChannels();c<b.getNumChannels();++c) b.clear(c,0,b.getNumSamples());
    EqDSP::Gains g; for(size_t i=0;i<7;++i) g[i]=raw[i]->load();
    dsp.process(b.getArrayOfWritePointers(),std::min(b.getNumChannels(),getTotalNumInputChannels()),b.getNumSamples(),g,raw[7]->load(),bypass);
}
void GraphicEQProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) { render(b,bypassValue->load()>0.5f); }
void GraphicEQProcessor::processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) { render(b,true); }
void GraphicEQProcessor::getStateInformation(juce::MemoryBlock& b) { auto xml=state.copyState().createXml(); copyXmlToBinary(*xml,b); }
void GraphicEQProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName(state.state.getType())) {
        auto tree=juce::ValueTree::fromXml(*xml);
        // Reject malformed/out-of-range state before it can reach DSP.
        for(auto child:tree) {
            const auto id=child.getProperty("id").toString();
            if(auto* p=state.getParameter(id)) {
                const float value=child.getProperty("value");
                if(!std::isfinite(value)) return;
                child.setProperty("value",p->convertFrom0to1(p->convertTo0to1(value)),nullptr);
            }
        }
        state.replaceState(tree);
    }
}
class PlainLook final : public blp::EditorLookAndFeel {
public:
    int getSliderThumbRadius(juce::Slider&) override { return 12; }
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override {
        juce::Slider::SliderLayout layout;
        layout.sliderBounds=slider.getLocalBounds().withTrimmedBottom(22);
        layout.textBoxBounds=slider.getLocalBounds().removeFromBottom(22);
        return layout;
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,
                          juce::Slider::SliderStyle,juce::Slider&) override {
        const float cx=float(x)+float(w)*0.5f;
        g.setColour(juce::Colour(0xffb9b8c0));
        g.fillRoundedRectangle(cx-4,float(y),8,float(h),4);
        g.setColour(juce::Colour(0xffb9b8c0));
        g.drawRoundedRectangle(cx-4,float(y),8,float(h),4,1);
        g.setColour(juce::Colour(0x26151518));
        g.fillRoundedRectangle(cx-9,pos-9,20,22,3);
        g.setColour(juce::Colour(0xfff6f5f7));
        g.fillRoundedRectangle(cx-10,pos-11,20,22,3);
        g.setColour(juce::Colour(0xfff6f5f7));
        g.drawRoundedRectangle(cx-10,pos-11,20,22,3,1);
        g.setColour(juce::Colour(0xff323238));
        g.drawLine(cx-6,pos,cx+6,pos,2);
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) override {
        auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(5);
        auto radius=std::min(r.getWidth(),r.getHeight())*0.5f;
        auto centre=r.getCentre();
        g.setColour(juce::Colour(0xfff6f5f7)); g.fillEllipse(centre.x-radius,centre.y-radius,radius*2,radius*2);
        g.setColour(juce::Colour(0xff323238));
        const float angle=start+pos*(end-start);
        g.drawLine(centre.x+std::sin(angle)*radius*0.42f,centre.y-std::cos(angle)*radius*0.42f,
                   centre.x+std::sin(angle)*radius*0.85f,centre.y-std::cos(angle)*radius*0.85f,2.5f);
    }
};
class Editor final : public blp::ThemedEditor {
public:
    explicit Editor(GraphicEQProcessor& p):blp::ThemedEditor(p) {
        setLookAndFeel(&look);
        const char* names[]={"100 Hz","200 Hz","400 Hz","800 Hz","1.6 kHz","3.2 kHz","6.4 kHz","Output"};
        const char* displayNames[]={"100","200","400","800","1.6k","3.2k","6.4k",""};
        for(size_t i=0;i<8;++i) {
            auto& k=knobs[i]; addAndMakeVisible(k);
            k.setSliderStyle(i<7?juce::Slider::LinearVertical:juce::Slider::RotaryHorizontalVerticalDrag);
            k.setTextBoxStyle(i<7?juce::Slider::NoTextBox:juce::Slider::TextBoxBelow,false,62,20);
            if(i<7) k.setPopupDisplayEnabled(true,true,this,-1);
            k.setTextValueSuffix(" dB"); k.setDoubleClickReturnValue(true,0);
            k.setColour(juce::Slider::textBoxTextColourId,blp::textWhite);
            k.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
            k.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
            k.setName(names[i]);
            labels[i].setText(displayNames[i],juce::dontSendNotification);
            labels[i].setJustificationType(juce::Justification::centred);
            labels[i].setColour(juce::Label::textColourId,blp::textWhite);
            addAndMakeVisible(labels[i]);
            attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,GraphicEQProcessor::ids[i],k);
        }
        addAndMakeVisible(bypass);
        bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
        setSize(480, 270);
    }
    ~Editor() override { setLookAndFeel(nullptr); }
    void paint(juce::Graphics& g) override {
        g.fillAll(blp::canvas); g.setColour(juce::Colour(0xfff6f5f7));
        g.setFont(17.0f); 
        g.setFont(12.0f);
        const char* ticks[]={"+15","0","-15"};
        for(int i=0;i<3;++i) blp::drawText(g, ticks[i],8,52+i*56,30,16,juce::Justification::centredRight);
        g.setFont(11.0f); blp::drawText(g, "dB",10,184,28,18,juce::Justification::centredRight);
        blp::paintHeader(g, getWidth(), "BLP Graphic EQ");
}
    void resized() override {
        for(int i=0;i<7;++i) { labels[size_t(i)].setBounds(48+i*60,186,60,20); knobs[size_t(i)].setBounds(48+i*60,52,60,150); }
        labels[7].setBounds(275,222,64,20); knobs[7].setBounds(210,205,60,60);
        bypass.setBounds(blp::bypassBounds(getWidth()));
    }
private:
    PlainLook look;
    std::array<juce::Slider,8> knobs;
    std::array<juce::Label,8> labels;
    blp::BypassButton bypass;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,8> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
};
juce::AudioProcessorEditor* GraphicEQProcessor::createEditor() { return new Editor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GraphicEQProcessor(); }
