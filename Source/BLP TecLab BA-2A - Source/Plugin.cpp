#include "BlpEditorTheme.h"
#include <JuceHeader.h>
class Processor;
class Editor;
class Processor : public juce::AudioProcessor {
public:
 juce::AudioProcessorValueTreeState state;
 double sr=44100; float envelope=0, reduction=0, memory=0;
 juce::SmoothedValue<float> squeeze,gain,mix,output,mode,bypass;
 int preset=0;
 static auto layout(){juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto add=[&](const char* id,const char* name,float lo,float hi,float def){p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,0.01f),def));};
 add("squeeze","Squeeze",0,100,70);add("gain","Gain",0,100,60);add("mix","Mix",0,100,100);add("output","Output",-18,18,0);
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Mode",juce::StringArray{"Compress","Limit"},0));p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));return p;}
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"BLP_STATE",layout()){}
 float v(const char* id){return state.getRawParameterValue(id)->load();}
 const juce::String getName()const override{return "BLP TecLab BA-2A";} bool acceptsMidi()const override{return false;} bool producesMidi()const override{return false;} double getTailLengthSeconds()const override{return 0;} bool isMidiEffect()const override{return false;}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==l.getMainOutputChannelSet()&&(l.getMainOutputChannelSet()==juce::AudioChannelSet::mono()||l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());}
 void prepareToPlay(double rate,int)override{sr=rate;envelope=reduction=memory=0; for(auto* s:{&squeeze,&gain,&mix,&output,&mode,&bypass})s->reset(rate,0.03);squeeze.setCurrentAndTargetValue(v("squeeze"));gain.setCurrentAndTargetValue(v("gain"));mix.setCurrentAndTargetValue(v("mix")/100);output.setCurrentAndTargetValue(v("output"));mode.setCurrentAndTargetValue(v("mode"));bypass.setCurrentAndTargetValue(v("bypass"));}
 void releaseResources()override{}
 juce::AudioProcessorParameter* getBypassParameter()const override{return state.getParameter("bypass");}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{juce::ScopedNoDenormals no;
 squeeze.setTargetValue(v("squeeze"));gain.setTargetValue(v("gain"));mix.setTargetValue(v("mix")/100);output.setTargetValue(v("output"));mode.setTargetValue(v("mode"));bypass.setTargetValue(v("bypass"));
 for(int i=0;i<b.getNumSamples();++i){float peak=0;for(int c=0;c<b.getNumChannels();++c)peak=std::max(peak,std::abs(b.getSample(c,i)));
 float e=std::exp(-1.0/(sr*(peak>envelope?0.003:0.055)));envelope=e*envelope+(1-e)*peak;
 float s=squeeze.getNextValue(), m=mode.getNextValue();float threshold=-3-0.43f*s, ratio=3+17*m;float over=juce::Decibels::gainToDecibels(envelope,-120.f)-threshold;
 float knee=8;float gr=over<-knee/2?0:over>knee/2?over*(1-1/ratio):(over+knee/2)*(over+knee/2)/(2*knee)*(1-1/ratio);gr*=std::min(1.f,s/8);
 memory+=float(1-std::exp(-1/(sr*1.5)))*(reduction-memory);float time=gr>reduction?0.012-0.007*m:0.08+0.7*std::min(1.f,memory/18);float a=std::exp(-1/(sr*time));reduction=a*reduction+(1-a)*gr;
 // Makeup follows actual compression, so Squeeze cannot become an upward expander.
 float active=juce::jlimit(0.f,1.f,s/8.f);float appliedReduction=reduction*active;float makeupDb=(-12+0.36f*gain.getNextValue())*std::min(1.f,appliedReduction/24.f);
 float g=juce::Decibels::decibelsToGain(makeupDb-appliedReduction),wet=mix.getNextValue(),out=juce::Decibels::decibelsToGain(output.getNextValue()),bp=bypass.getNextValue();
 for(int c=0;c<b.getNumChannels();++c){float dry=b.getSample(c,i),y=(dry*(1-wet)+dry*g*wet)*out; // Smooth safety saturation above -1 dBFS.
 const float edge=0.89125f;if(std::abs(y)>edge){float safe=std::copysign(edge+(1-edge)*std::tanh((std::abs(y)-edge)/(1-edge)),y);y+=active*(safe-y);}
 b.setSample(c,i,y*(1-bp)+dry*bp);}}
 }
 int getNumPrograms()override{return 4;}int getCurrentProgram()override{return preset;}const juce::String getProgramName(int i)override{return juce::StringArray{"Gentle Vocal","Smooth Leveling","Rap Vocal","Strong Compression"}[juce::jlimit(0,3,i)];}void changeProgramName(int,const juce::String&)override{}
 void setCurrentProgram(int i)override{preset=juce::jlimit(0,3,i);const float values[4][5]={{40,40,100,0,0},{70,60,100,0,0},{78,62,90,-1,0},{85,65,100,-2,1}};int j=0;for(auto id:{"squeeze","gain","mix","output","mode"}){auto* p=state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(values[preset][j++]));}}
 void getStateInformation(juce::MemoryBlock& dest)override{auto tree=state.copyState();tree.setProperty("preset",preset,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,dest);}void setStateInformation(const void* d,int n)override{auto xml=getXmlFromBinary(d,n);if(xml&&xml->hasTagName("BLP_STATE")){auto t=juce::ValueTree::fromXml(*xml);preset=(int)t.getProperty("preset",0);state.replaceState(t);}}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
};
class ThinKnobLook : public blp::EditorLookAndFeel {
public:
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                       float position,float start,float end,juce::Slider&) override {
  const float cx=x+width*0.5f, cy=y+height*0.5f;
  const float radius=juce::jmin(28.f,juce::jmin(width,height)*0.5f-3.f);
  const float angle=start+position*(end-start);
  const juce::PathStrokeType stroke(2.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
  juce::Path track;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);
  g.setColour(juce::Colour(0xff47474f));g.strokePath(track,stroke);
  if(position>0){juce::Path active;active.addCentredArc(cx,cy,radius,radius,0,start,angle,true);
   g.setColour(juce::Colour(0xffee1c25));g.strokePath(active,stroke);}
  const float face=radius-5.f;
  g.setColour(juce::Colour(0xfff6f5f7));g.fillEllipse(cx-face,cy-face,face*2,face*2);
  juce::Path marker;marker.startNewSubPath(cx+std::sin(angle)*(face-13.f),cy-std::cos(angle)*(face-13.f));
  marker.lineTo(cx+std::sin(angle)*(face-5.f),cy-std::cos(angle)*(face-5.f));
  g.setColour(juce::Colour(0xff323238));g.strokePath(marker,juce::PathStrokeType(2.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 }
};
class Editor:public blp::ThemedEditor {
 Processor& p;ThinKnobLook look;juce::Slider knobs[2];juce::Label labels[2];blp::MenuBox mode { "Mode" };blp::BypassButton bypass;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ma;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
 public:Editor(Processor& proc):blp::ThemedEditor(proc),p(proc){setSize(300, 270);look.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffee1c25));look.setColour(juce::Slider::thumbColourId,juce::Colours::white);look.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setLookAndFeel(&look);
 const char* ids[]={"gain","squeeze"};const char* names[]={"Gain","Squeeze"};float defaults[]={60,70};for(int i=0;i<2;i++){addAndMakeVisible(knobs[i]);knobs[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);knobs[i].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);knobs[i].setTextValueSuffix(" %");knobs[i].setPopupDisplayEnabled(false,true,this);knobs[i].setDoubleClickReturnValue(true,defaults[i]);knobs[i].setBounds(40+i*130,65,90,82);labels[i].setText(names[i],juce::dontSendNotification);labels[i].setColour(juce::Label::textColourId,blp::textWhite);labels[i].setJustificationType(juce::Justification::centred);labels[i].setBounds(40+i*130,151,90,20);addAndMakeVisible(labels[i]);attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],knobs[i]));}
 mode.addItem("Compress",1);mode.addItem("Limit",2);addAndMakeVisible(mode);mode.setBounds(62,212,176,34);ma=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);
 addAndMakeVisible(bypass);bypass.setBounds(blp::bypassBounds(getWidth()));ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);}
 ~Editor() override { setLookAndFeel(nullptr); }
 void paint(juce::Graphics& g) override
 {
     g.fillAll(blp::canvas);
     blp::paintHeader(g, getWidth(), "BLP TecLab BA-2A");
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}



