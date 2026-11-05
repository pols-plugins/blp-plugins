#include "BlpEditorTheme.h"
#include <juce_audio_utils/juce_audio_utils.h>
using namespace juce;

static const StringArray divisions{"1/2","1/4\xC2\xB7","1/4","1/4T","1/8\xC2\xB7","1/8","1/8T","1/16\xC2\xB7","1/16","1/16T"};
static constexpr double ratios[]{2,1.5,1,2.0/3,.75,.5,1.0/3,.375,.25,1.0/6};
struct Control { const char* id; const char* label; const char* unit; float lo, hi, initial; };
static constexpr Control controls[]{
    {"feedback","Feedback","%",0,100,20}, {"ms","Time","ms",10,2600,500},
    {"hp","High Pass","Hz",50,1200,50}, {"lp","Low Pass","Hz",1200,16000,16000},
    {"spread","Spread","%",0,100,50}, {"wet","Wet","%",0,100,50},
    {"dry","Dry","%",0,100,100}, {"volume","Volume","dB",-12,12,0}
};
using State = AudioProcessorValueTreeState;
static State::ParameterLayout layout()
{
    State::ParameterLayout result;
    for (auto c : controls)
        result.add(std::make_unique<AudioParameterFloat>(ParameterID{c.id,1},
            String(String(c.id) == "ms" ? "MS Time" : c.label),
            NormalisableRange<float>{c.lo,c.hi,1},c.initial,
            AudioParameterFloatAttributes().withLabel(c.unit)));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"mode",1},"Time Mode",StringArray{"BEAT","MS"},0));
    result.add(std::make_unique<AudioParameterChoice>(ParameterID{"beat",1},"Beat Time",divisions,2));
    result.add(std::make_unique<AudioParameterBool>(ParameterID{"bypass",1},"Bypass",false));
    return result;
}

class Delay final : public AudioProcessor
{
public:
    Delay() : AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true)
                             .withOutput("Output",AudioChannelSet::stereo(),true)),
              state(*this,nullptr,"DDelay",layout())
    {
        for (size_t i=0;i<8;++i) values[i]=state.getRawParameterValue(controls[i].id);
        mode=state.getRawParameterValue("mode");
        beat=state.getRawParameterValue("beat");
    }
    State state;
    AudioProcessorParameter* getBypassParameter() const override { return state.getParameter("bypass"); }
    const String getName() const override { return "BLP D-Delay+"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3600; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int,const String&) override {}
    bool hasEditor() const override { return true; }
    AudioProcessorEditor* createEditor() override;
    bool isBusesLayoutSupported(const BusesLayout& b) const override
    {
        return b.getMainInputChannelSet()==AudioChannelSet::stereo()
            && b.getMainOutputChannelSet()==AudioChannelSet::stereo();
    }
    void getStateInformation(MemoryBlock& data) override
    {
        if (auto xml=state.copyState().createXml()) copyXmlToBinary(*xml,data);
    }
    void setStateInformation(const void* data,int size) override
    {
        if (auto xml=getXmlFromBinary(data,size); xml && xml->hasTagName(state.state.getType()))
            state.replaceState(ValueTree::fromXml(*xml));
    }
    void prepareToPlay(double sampleRate,int) override
    {
        rate=sampleRate;
        ring.setSize(2,int(std::ceil(32*rate))+2);
        ring.clear();
        write=0;
        current=0;
        remaining=0;
        fadeLength=jmax(1,int(rate*.025));
        for (int i=0;i<8;++i)
        {
            smooth[i].reset(rate,.02);
            smooth[i].setCurrentAndTargetValue(values[i]->load());
        }
        bypassMix.reset(rate,.02);
        bypassMix.setCurrentAndTargetValue(state.getRawParameterValue("bypass")->load());
        hpState.fill(0);
        lpState.fill(0);
    }
    void releaseResources() override {}
    void processBlock(AudioBuffer<float>& audio,MidiBuffer&) override
    {
        ScopedNoDenormals noDenormals;
        double bpm=120;
        if (auto* head=getPlayHead())
            if (auto position=head->getPosition())
                if (auto tempo=position->getBpm(); tempo && std::isfinite(*tempo) && *tempo>0)
                    bpm=*tempo;
        const int length=ring.getNumSamples();
        const double requested=mode->load()>=.5f ? values[1]->load()
            : 60000.0/bpm*ratios[jlimit(0,9,int(beat->load()))];
        const double target=jlimit(1.0,double(length-2),requested*rate*.001);
        if (current==0) current=target;
        for (int i=0;i<8;++i) smooth[i].setTargetValue(values[i]->load());

        bypassMix.setTargetValue(state.getRawParameterValue("bypass")->load());
        for (int n=0;n<audio.getNumSamples();++n)
        {
            if (remaining==0 && std::abs(target-current)>.001)
            {
                next=target;
                remaining=fadeLength;
            }
            const float mix=remaining ? 1.0f-float(remaining)/float(fadeLength) : 0;
            const float feedback=smooth[0].getNextValue()*.01f;
            const float high=coefficient(smooth[2].getNextValue());
            const float low=coefficient(smooth[3].getNextValue());
            const float spread=smooth[4].getNextValue()*.01f;
            const float wet=smooth[5].getNextValue()*.01f;
            const float dry=smooth[6].getNextValue()*.01f;
            const float gain=Decibels::decibelsToGain(smooth[7].getNextValue());

            const float bypass=bypassMix.getNextValue();
            for (int ch=0;ch<2;++ch)
            {
                auto* samples=ring.getWritePointer(ch);
                const auto read=[&](double center)
                {
                    const double offset=spread*jmin(center*.25,center-1,length-2-center);
                    double index=write-center+(ch==0 ? offset : -offset);
                    if (index<0) index+=length;
                    const int a=int(index);
                    return samples[a]+float(index-a)*(samples[(a+1)%length]-samples[a]);
                };
                float delayed=read(current);
                if (remaining) delayed+=mix*(read(next)-delayed);
                hpState[ch]+=high*(delayed-hpState[ch]);
                lpState[ch]+=low*(delayed-hpState[ch]-lpState[ch]);
                const float input=audio.getSample(ch,n);
                samples[write]=jlimit(-4.0f,4.0f,input+feedback*lpState[ch]);
                const float effected=gain*(dry*input+wet*lpState[ch]);
                audio.setSample(ch,n,effected+bypass*(input-effected));
            }
            if (remaining && --remaining==0) current=next;
            if (++write==length) write=0;
        }
    }
private:
    float coefficient(float hz) const
    {
        return float(-std::expm1(-MathConstants<double>::twoPi*jmin(double(hz),rate*.49)/rate));
    }
    std::atomic<float>* values[8]{};
    std::atomic<float>* mode{};
    std::atomic<float>* beat{};
    SmoothedValue<float> smooth[8],bypassMix;
    AudioBuffer<float> ring;
    std::array<float,2> hpState{},lpState{};
    double rate=48000,current=0,next=0;
    int write=0,remaining=0,fadeLength=1;
};

class Editor final : public blp::ThemedEditor, private Timer
{
public:
    explicit Editor(Delay& p) : blp::ThemedEditor(p), processor(p)
    {
        setOpaque(true);
        for (int i=0;i<8;++i)
        {
            auto c=controls[i];
            auto& s=sliders[i];
            s.setName(c.label);
            s.setSliderStyle(Slider::RotaryVerticalDrag);
            if (i==0) s.setMouseDragSensitivity(110);
            s.setTextBoxStyle(Slider::NoTextBox,false,0,0);
            s.setDoubleClickReturnValue(true,c.initial);
            addAndMakeVisible(s);
            attachments[i]=std::make_unique<State::SliderAttachment>(p.state,c.id,s);
        }
        modes.addItemList({"BEAT","MS"},1);
        notes.setSliderStyle(Slider::RotaryVerticalDrag);
        notes.setTextBoxStyle(Slider::NoTextBox,false,0,0);
        notes.setDoubleClickReturnValue(true,2);
        notes.setMouseDragSensitivity(108);
        addAndMakeVisible(notes);
        modes.setColour(ComboBox::textColourId,Colours::transparentBlack);
        addAndMakeVisible(modes);
        modes.setName("Time mode");
        notes.setName("Beat division");
        modes.setSelectedId(isMs() ? 2 : 1,dontSendNotification);
        modes.onDoubleClick=[this]{ modes.setSelectedId(1); };
        modeAttachment=std::make_unique<State::ComboBoxAttachment>(p.state,"mode",modes);
        noteAttachment=std::make_unique<State::SliderAttachment>(p.state,"beat",notes);
        msEntry.setColour(TextEditor::backgroundColourId,Colour(0xff323238));
        msEntry.setColour(TextEditor::textColourId,blp::textWhite);
        msEntry.setInputRestrictions(4,"0123456789");
        msEntry.onReturnKey=[this]{ commitMs(); };
        msEntry.onFocusLost=[this]{ commitMs(); };
        msEntry.onEscapeKey=[this]{ msEntry.setVisible(false); };
        addChildComponent(msEntry);
        editButton.setTitle("Edit delay time in milliseconds");
        editButton.setMouseCursor(MouseCursor::IBeamCursor);
        addAndMakeVisible(editButton);
        editButton.onClick=[this]
        {
            msEntry.setText(String(roundToInt(sliders[1].getValue())),false);
            msEntry.setVisible(true);
            msEntry.toFront(true);
            msEntry.grabKeyboardFocus();
            msEntry.selectAll();
        };
        addAndMakeVisible(bypassButton);
        bypassAttachment=std::make_unique<State::ButtonAttachment>(p.state,"bypass",bypassButton);
        setSize(356, 270);
        animatedFeedbackOffset=float((sliders[0].getValue()-20.0)*.36);
        timerCallback();
        startTimerHz(30);
    }

    void resized() override
    {
        sliders[0].setBounds(0,56,255,77);
        bypassButton.setBounds(blp::bypassBounds(getWidth()));
        modes.setBounds(85,143,44,19);
        notes.setBounds(15,157,16,29);
        editButton.setBounds(31,164,94,23);
        msEntry.setBounds(31,164,94,23);
        sliders[1].setBounds(15,157,16,29);
        for (int i=2;i<8;++i) sliders[i].setBounds(cell(i));
    }
    void paint(Graphics& g) override
    {
        g.fillAll(blp::canvas);
        const float pillY=85.0f-animatedFeedbackOffset;
        const int feedbackY=roundToInt(pillY+16);
        {
            Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(0, 34, getWidth(), 107);
            const float amount = jlimit(0.0f, 1.0f, (animatedFeedbackOffset / .36f + 20.0f) * .01f);
            const float centreY = 91.0f + amount * 16.0f;
            const float maxRadius = 23.0f + amount * 152.0f;
            const float phase = float(Time::getMillisecondCounterHiRes() * 0.0031);
            const int rings = 4 + roundToInt(amount * 24.0f);
            for (int i = 0; i < rings; ++i)
            {
                const float fraction = float(i + 1) / float(rings);
                const float wave = std::sin(phase - fraction * 9.0f);
                const float radius = maxRadius * fraction * (1.0f + 0.075f * amount * wave);
                const float opacity = 0.13f + amount * (0.20f + 0.14f * wave);
                g.setColour(blp::accent.withAlpha(opacity));
                g.drawEllipse(178.0f - radius, centreY - radius * .52f, radius * 2.0f,
                              radius * 1.04f, 0.75f + amount * 0.4f);
            }
        }
        g.setFont(FontOptions(13));
        g.setColour(Colours::white);
        



        g.setColour(Colour(0xffee1c25));
        g.setColour(Colours::white); g.drawHorizontalLine(feedbackY,0,356);
        g.setColour(Colour(0xff151518));
        g.fillRoundedRectangle(125,pillY,107,32,16);
        g.fillRect(5,roundToInt(pillY)-2,46,16);
        g.setColour(Colours::white);
        g.drawRoundedRectangle(125,pillY,107,32,16,1);
        blp::drawText(g, "Feedback",125,roundToInt(pillY),98,32,Justification::centred);
        g.drawLine(213,pillY+15,216,pillY+12,1);
        g.drawLine(216,pillY+12,219,pillY+15,1);
        g.drawLine(213,pillY+17,216,pillY+20,1);
        g.drawLine(216,pillY+20,219,pillY+17,1);
        g.setFont(FontOptions(10)); blp::drawText(g, value(0),5,roundToInt(pillY)-2,46,16,Justification::centredLeft);

        g.setColour(Colour(0xff323238));
        g.fillRoundedRectangle(modes.getBounds().toFloat().reduced(1,3),3);
        g.setColour(Colours::white);
        g.setFont(FontOptions(12));
        blp::drawText(g, isMs() ? "MS" : "beat",modes.getBounds(),Justification::centred);
        if (!isMs())
        {
            g.setFont(FontOptions(16));
            blp::drawText(g, divisions[noteIndex()],32,165,66,21,Justification::centredLeft);
        }
        for (int i=1;i<8;++i)
        {
            const auto area=cell(i);
            g.setColour(Colour(0xffb9b8c0));
            g.setFont(FontOptions(12));
            blp::drawText(g, controls[i].label,area.withHeight(19).translated(10,0),Justification::centredLeft);

            const float proportion=float(i==1 && !isMs() ? noteIndex()/9.0 : sliders[i].valueToProportionOfLength(sliders[i].getValue()));
            const float top=float(area.getBottom()-25);
            g.setColour(Colour(0xff47474f));
            g.fillRect(float(area.getX()),top,2.0f,21.0f);
            g.setColour(Colour(0xffee1c25));
            g.fillRect(float(area.getX()),top+20*(1-proportion),1.0f,1+20*proportion);
            g.setColour(Colours::white);
            g.setFont(FontOptions(16.0f));
            if (i!=1 || isMs()) blp::drawText(g, value(i),area.withTrimmedTop(22).translated(10,0),Justification::centredLeft);
        }
        blp::paintHeader(g, getWidth(), "BLP D-Delay+");
}
private:
    bool isMs() const { return processor.state.getRawParameterValue("mode")->load() >= .5f; }
    int noteIndex() const { return jlimit(0,9,int(processor.state.getRawParameterValue("beat")->load())); }
    struct ResetBox : ComboBox
    {
        void paint(Graphics&) override {}
        std::function<void()> onDoubleClick;
        void mouseDoubleClick(const MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }
    };
    Rectangle<int> cell(int i) const
    {
        if (i==1) return {22,144,108,42};
        if (i==2) return {140,144,92,42};
        if (i==3) return {238,144,110,42};
        constexpr int x[]{22,107,185,263};
        return {x[i-4],200,80,42};
    }
    String value(int i) const { return String(roundToInt(sliders[i].getValue()))+" "+controls[i].unit; }
    void timerCallback() override
    {
        const bool ms=isMs();




        notes.setVisible(!ms);
        sliders[1].setVisible(ms);
        editButton.setVisible(ms);
        if (!ms) msEntry.setVisible(false);
        const auto target=float((sliders[0].getValue()-20.0)*.36);
        animatedFeedbackOffset+=.45f*(target-animatedFeedbackOffset);
        repaint();
    }
    void commitMs()
    {
        if (!msEntry.isVisible()) return;
        const auto text=msEntry.getText();
        msEntry.setVisible(false);
        if (text.isNotEmpty()) sliders[1].setValue(text.getIntValue(),sendNotificationSync);
    }
    struct InvisibleSlider : Slider { void paint(Graphics&) override {} };
    Delay& processor;
    InvisibleSlider sliders[8];
    ResetBox modes;
    InvisibleSlider notes;
    TextEditor msEntry;
    struct ReadoutButton : TextButton { void paintButton(Graphics&,bool,bool) override {} };
    ReadoutButton editButton;
    blp::BypassButton bypassButton;
    float animatedFeedbackOffset=0;
    std::unique_ptr<State::ButtonAttachment> bypassAttachment;
    std::unique_ptr<State::SliderAttachment> attachments[8];
    std::unique_ptr<State::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<State::SliderAttachment> noteAttachment;
};

AudioProcessorEditor* Delay::createEditor() { return new Editor(*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Delay; }









