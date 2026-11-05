#include "BlpEditorTheme.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <cmath>
class Processor : public juce::AudioProcessor {
public:
 static juce::AudioProcessorValueTreeState::ParameterLayout layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout l;
  auto add=[&](const char* id,const char* name,float lo,float hi,float def,float skew=1.f){l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,juce::String(id)=="freq"?1.f:(juce::String(id)=="threshold"?0.1f:0.001f),skew),def));};
  add("freq","Frequency",1000,16000,9000); add("threshold","Threshold",-40,0,-19); add("amount","Amount",0,100,60); add("attack","Attack",0.1f,20,2,0.4f); add("release","Release",10,300,80,0.5f); add("mix","Mix",0,100,100);
  l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false)); return l;
 }
 juce::AudioProcessorValueTreeState state{*this,nullptr,"DeEsserState",layout()};
 std::array<const char*,6> ids{{"freq","threshold","amount","attack","release","mix"}};
 std::array<juce::SmoothedValue<float>,7> smooth;
 struct Filter {double z1=0,z2=0; float run(float x,double b0,double b1,double b2,double a1,double a2){double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return (float)y;}};
 std::array<Filter,2> filters;
 double rate=44100; float reduction=0;
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){}
 const juce::String getName() const override{return "BLP DeEsser";}
 bool acceptsMidi() const override{return false;} bool producesMidi() const override{return false;} double getTailLengthSeconds() const override{return 0;}
 bool isBusesLayoutSupported(const BusesLayout& b) const override{return b.getMainInputChannelSet()==b.getMainOutputChannelSet()&&(b.getMainOutputChannelSet()==juce::AudioChannelSet::mono()||b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo());}
 void prepareToPlay(double sr,int) override {rate=sr;filters={};reduction=0;for(int i=0;i<7;++i){smooth[i].reset(sr,0.02);smooth[i].setCurrentAndTargetValue(state.getRawParameterValue(i<6?ids[i]:"bypass")->load());}}
 void releaseResources() override{}
 juce::AudioProcessorParameter* getBypassParameter() const override{return state.getParameter("bypass");}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) override{process(b,false);}
 void processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&) override{process(b,true);}
 void process(juce::AudioBuffer<float>& b,bool bypass){
  juce::ScopedNoDenormals nd;for(int i=0;i<7;++i)smooth[i].setTargetValue(i==6&&bypass?1.f:state.getRawParameterValue(i<6?ids[i]:"bypass")->load());
  for(int n=0;n<b.getNumSamples();++n){
   float f=smooth[0].getNextValue(),t=smooth[1].getNextValue(),amount=smooth[2].getNextValue()/100,attack=smooth[3].getNextValue(),release=smooth[4].getNextValue(),mix=smooth[5].getNextValue()/100,by=smooth[6].getNextValue();
   // Constant-peak bandpass, Q=0.8. Subtract only this region from dry audio.
   double w=juce::MathConstants<double>::twoPi*juce::jmin(double(f),rate*0.45)/rate, alpha=std::sin(w)/1.6,a0=1+alpha;
   double b0=alpha/a0,a1=-2*std::cos(w)/a0,a2=(1-alpha)/a0;
   float band[2]{},peak=0;for(int c=0;c<b.getNumChannels();++c){band[c]=filters[c].run(b.getSample(c,n),b0,0,-b0,a1,a2);peak=juce::jmax(peak,std::abs(band[c]));}
   float over=juce::jmax(0.f,juce::Decibels::gainToDecibels(peak,-120.f)-t);
   float target=juce::jmin(18.f,over*0.85f)*amount;
   float coeff=(float)std::exp(-1.0/(rate*(target>reduction?attack:release)*0.001));reduction=target+coeff*(reduction-target);
   float cut=(1-juce::Decibels::decibelsToGain(-reduction))*mix*(1-by);
   for(int c=0;c<b.getNumChannels();++c)b.setSample(c,n,b.getSample(c,n)-cut*band[c]);
  }
 }
 int getNumPrograms() override{return 1;} int getCurrentProgram() override{return 0;} void setCurrentProgram(int) override{} const juce::String getProgramName(int) override{return "Vocal Default";} void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock& dest) override {auto xml=state.copyState().createXml();copyXmlToBinary(*xml,dest);}
 void setStateInformation(const void* data,int size) override {auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName(state.state.getType()))state.replaceState(juce::ValueTree::fromXml(*xml));}
 bool hasEditor() const override{return true;} juce::AudioProcessorEditor* createEditor() override;
};
class GraphicLook : public blp::EditorLookAndFeel {
public:
 void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& slider) override {
  if(style==juce::Slider::LinearVertical){g.setColour(juce::Colours::white.withAlpha(0.8f));g.drawHorizontalLine((int)pos,(float)x,(float)(x+w));juce::Rectangle<float> pill(x+w*0.5f-54,pos-16,108,32);g.setColour(juce::Colour(0xff151518));g.fillRoundedRectangle(pill,16);g.setColour(juce::Colours::white);g.drawRoundedRectangle(pill,16,1);g.setFont(13.f);blp::drawText(g, "Threshold",pill.translated(-5.0f, 0.0f),juce::Justification::centred);blp::paintAdjustArrows(g,pill);g.setFont(11.f);blp::drawText(g, juce::String(slider.getValue(),1)+" dB",juce::Rectangle<float>((float)x+2,juce::jmax(0.f,pos-17.f),74.f,16.f),juce::Justification::centredLeft);}
  else{g.setColour(juce::Colour(0xff323238));g.fillRect(x,y+h/2,w,2);g.setColour(juce::Colour(0xffee1c25));g.fillRect(x,y+h/2,juce::jmax(1,(int)pos-x),2);g.fillRect((int)pos-1,y+h/2-7,2,16);}
 }
};
class Editor:public blp::ThemedEditor, private juce::Timer {
 GraphicLook look;juce::Slider frequency,threshold;blp::BypassButton bypass;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fa,ta;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
public:
 Editor(Processor& p):blp::ThemedEditor(p){setLookAndFeel(&look);
 frequency.setSliderStyle(juce::Slider::LinearHorizontal);threshold.setSliderStyle(juce::Slider::LinearVertical);
 for(auto* s:{&frequency,&threshold}){s->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);s->setColour(juce::Slider::textBoxTextColourId,blp::textWhite);addAndMakeVisible(s);s->onValueChange=[this]{repaint();};}
 frequency.setTextBoxStyle(juce::Slider::TextBoxBelow,false,120,24);frequency.setNumDecimalPlacesToDisplay(0);frequency.setTextValueSuffix(" Hz");
 threshold.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);threshold.setNumDecimalPlacesToDisplay(1);threshold.setTextValueSuffix(" dB");
 fa=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"freq",frequency);ta=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"threshold",threshold);
 addAndMakeVisible(bypass);ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);setSize(280, 270); startTimerHz(30); }
 ~Editor() override{stopTimer();setLookAndFeel(nullptr);}
 void timerCallback() override { if (threshold.getValue() < 0.0) repaint(0, 58, getWidth(), 143); }
 void paint(juce::Graphics& g) override{g.fillAll(blp::canvas);g.setColour(juce::Colours::white);g.setFont(16.f);g.setFont(11.f);g.setColour(juce::Colour(0xffb9b8c0));
 const float centre = 20.0f + float((frequency.getValue() - 1000.0) / 15000.0) * 240.0f;
 const float strength = juce::jlimit(0.0f, 1.0f, float(threshold.getValue() / -40.0));
 const float phase = float(juce::Time::getMillisecondCounterHiRes() * 0.007);
 g.setColour(blp::accent.withAlpha(0.06f + strength * 0.16f));
 g.fillRect(centre - 14.0f, 65.0f, 28.0f, 131.0f);
 juce::Random dots(71);
 for (int i = 0; i < 210; ++i)
 {
     float x = 4.0f + dots.nextFloat() * 272.0f;
     float y = 68.0f + dots.nextFloat() * 128.0f;
     const float diameter = 1.5f + dots.nextFloat() * 2.5f;
     const bool highlighted = std::abs(x - centre) < 14.0f;
     if (highlighted)
     {
         x = juce::jlimit(centre - 14.0f, centre + 14.0f - diameter,
                          x + 5.0f * strength * std::sin(phase + i * 2.13f));
         y = juce::jlimit(65.0f, 196.0f - diameter,
                          y + 6.0f * strength * std::cos(phase * 1.3f + i * 0.81f));
     }
     g.setColour(highlighted ? blp::accent : juce::Colour(0xff323238));
     g.fillEllipse(x, y, diameter, diameter);
 }
 g.setColour(juce::Colour(0xffb9b8c0));blp::drawText(g, "Frequency",18,202,150,18,juce::Justification::centredLeft);    blp::paintHeader(g, getWidth(), "BLP DeEsser");
}
 void resized() override{threshold.setBounds(2,48,276,149);frequency.setBounds(18,220,244,40);bypass.setBounds(blp::bypassBounds(getWidth()));}
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);} juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}






