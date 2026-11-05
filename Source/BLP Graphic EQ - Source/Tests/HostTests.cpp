#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
#include "../Source/Plugin.cpp"
void check(bool b,const char* m) { if(!b) throw std::runtime_error(m); }
struct PresetVisitor : juce::ExtensionsVisitor {
    juce::MemoryBlock data;
    bool loading=false,ok=false;
    void visitVST3Client(const VST3Client& client) override {
        if(loading) ok=client.setPreset(data);
        else { data=client.getPreset(); ok=data.getSize()>0; }
    }
};
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    try {
        check(argc>=2,"VST3 path required");
        juce::VST3PluginFormat format;
        juce::OwnedArray<juce::PluginDescription> types;
        format.findAllTypesForFile(types,argv[1]); check(types.size()==1,"VST3 scan failed");
        check(types[0]->name=="BLP Graphic EQ","plugin name");
        check(types[0]->manufacturerName=="BLP","manufacturer name");
        juce::String error;
        auto p=format.createInstanceFromDescription(*types[0],48000,256,error);
        check(p!=nullptr,error.toRawUTF8());
        check(p->getParameters().size()>=9,"missing parameters");
        auto params=p->getParameters();
        for(auto* param:params) check(param->isAutomatable(),"non automatable parameter");
        auto findParam=[&](const juce::String& name) {
            for(auto* param:params) if(param->getName(100)==name) return param;
            throw std::runtime_error("Parameter not found: "+name.toStdString());
        };
        for(int ch:{1,2}) for(double sr:{44100.,48000.,88200.,96000.}) {
            auto layout=p->getBusesLayout();
            layout.inputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(ch));
            layout.outputBuses.set(0,juce::AudioChannelSet::canonicalChannelSet(ch));
            check(p->setBusesLayout(layout),"mono/stereo layout rejected");
            p->setRateAndBufferSizeDetails(sr,256); p->prepareToPlay(sr,256);
            check(p->getLatencySamples()==0,"unexpected latency");
            juce::AudioBuffer<float> b(ch,256); juce::MidiBuffer midi;
            for(auto* param:params) param->setValueNotifyingHost(param->getDefaultValue());
            for(int block=0;block<100;++block) {
                for(int c=0;c<ch;++c) for(int n=0;n<256;++n) b.setSample(c,n,float(0.01*std::sin((n+block*256)*0.12)));
                p->processBlock(b,midi);
                for(int c=0;c<ch;++c) for(int n=0;n<256;++n) check(std::isfinite(b.getSample(c,n)),"invalid audio");
            }
            p->releaseResources();
            std::cout<<"PASS VST3 audio "<<ch<<" channel(s), "<<sr<<" Hz\n";
        }
        p->setRateAndBufferSizeDetails(48000,256); p->prepareToPlay(48000,256);
        const char* names[]={"100 Hz","200 Hz","400 Hz","800 Hz","1.6 kHz","3.2 kHz","6.4 kHz"};
        auto measure=[&](double frequency) {
            juce::AudioBuffer<float> b(2,256); juce::MidiBuffer midi;
            double in=0,out=0;
            for(int block=0;block<400;++block) {
                for(int n=0;n<256;++n) for(int c=0;c<2;++c)
                    b.setSample(c,n,float(0.01*std::sin(6.283185307179586*frequency*(block*256+n)/48000)));
                juce::AudioBuffer<float> dry; dry.makeCopyOf(b);
                p->processBlock(b,midi);
                if(block>200) for(int n=0;n<256;++n) { in+=dry.getSample(0,n)*dry.getSample(0,n);out+=b.getSample(0,n)*b.getSample(0,n); }
            }
            return 10*std::log10(out/in);
        };
        for(int band=0;band<7;++band) for(float norm:{0.0f,1.0f}) {
            for(auto* param:params) param->setValueNotifyingHost(param->getDefaultValue());
            auto* param=findParam(names[band]); param->beginChangeGesture();param->setValueNotifyingHost(norm);param->endChangeGesture();
            check(std::abs(measure(EqDSP::frequencies[size_t(band)])-(norm==0?-15:15))<0.05,"VST3 automated band response");
        }
        for(auto* param:params) param->setValueNotifyingHost(param->getDefaultValue());
        for(float norm:{0.0f,1.0f}) {
            findParam("Output")->setValueNotifyingHost(norm);
            check(std::abs(measure(997)-(norm==0?-15:15))<0.05,"VST3 output gain");
        }
        findParam("Bypass")->setValueNotifyingHost(1);
        check(std::abs(measure(997))<0.001,"VST3 bypass");
        p->releaseResources();
        std::cout<<"PASS VST3 automated seven-band gains, Output and Bypass audio measurements\n";
        std::vector<float> saved;
        for(int i=0;i<params.size();++i) {
            const auto value=params[i]->getName(100)=="Bypass" ? float(i%2) : float(i%3)*0.3f;
            params[i]->setValueNotifyingHost(value); saved.push_back(params[i]->getValue());
        }
        p->prepareToPlay(48000,256);
        { juce::AudioBuffer<float> flush(2,256); flush.clear(); juce::MidiBuffer midi; p->processBlock(flush,midi); }
        juce::MemoryBlock state; p->getStateInformation(state); check(state.getSize()>0,"empty state");
        for(auto* param:params) param->setValueNotifyingHost(param->getDefaultValue());
        { juce::AudioBuffer<float> flush(2,256); flush.clear(); juce::MidiBuffer midi; p->processBlock(flush,midi); }
        p->setStateInformation(state.getData(),int(state.getSize()));
        for(int i=0;i<params.size();++i) {
            if(std::abs(params[i]->getValue()-saved[size_t(i)])>=0.0001f)
                std::cerr<<params[i]->getName(100)<<" expected "<<saved[size_t(i)]<<" got "<<params[i]->getValue()<<'\n';
            check(std::abs(params[i]->getValue()-saved[size_t(i)])<0.0001f,"state restore failed");
        }
        juce::File preset=juce::File::getCurrentWorkingDirectory().getChildFile("roundtrip.vstpreset");
        PresetVisitor visitor; p->getExtensions(visitor);
        check(visitor.ok&&preset.replaceWithData(visitor.data.getData(),visitor.data.getSize()),"preset save failed");
        for(auto* param:params) param->setValueNotifyingHost(param->getDefaultValue());
        { juce::AudioBuffer<float> flush(2,256); flush.clear(); juce::MidiBuffer midi; p->processBlock(flush,midi); }
        visitor.data.reset(); check(preset.loadFileAsData(visitor.data),"preset read failed");
        visitor.loading=true; visitor.ok=false; p->getExtensions(visitor);
        check(visitor.ok,"preset load failed");
        for(int i=0;i<params.size();++i) check(std::abs(params[i]->getValue()-saved[size_t(i)])<0.0001f,"preset restore failed");
        std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditorIfNeeded());
        check(editor!=nullptr,"editor missing");
        check(editor->getWidth()==480&&editor->getHeight()==300,"editor size");
        GraphicEQProcessor previewProcessor;
        std::unique_ptr<juce::AudioProcessorEditor> preview(previewProcessor.createEditor());
        int knobs=0,vertical=0,rotary=0;
        for(auto* child:preview->getChildren()) if(auto* slider=dynamic_cast<juce::Slider*>(child)) {
            ++knobs; check(slider->getDoubleClickReturnValue()==0.0,"knob reset value");
            check(slider->getValue()==0.0,"default knob gain");
            check(slider->getMinimum()==-15.0&&slider->getMaximum()==15.0,"control range");
            if(slider->getSliderStyle()==juce::Slider::LinearVertical) ++vertical;
            if(slider->getSliderStyle()==juce::Slider::RotaryHorizontalVerticalDrag) ++rotary;
        }
        check(knobs==8,"eight knobs required");
        check(vertical==7&&rotary==1,"seven sliders and one Level knob required");
        auto snapshot=preview->createComponentSnapshot(preview->getLocalBounds(),true,2.0f);
        juce::FileOutputStream png(juce::File::getCurrentWorkingDirectory().getChildFile("BLP-GraphicEQ-preview.png"));
        juce::PNGImageFormat().writeImageToStream(snapshot,png);
        std::cout<<"PASS automation parameters, project state, .vstpreset disk roundtrip, editor rendering\n";
        editor.reset(); p.reset(); return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n';return 1; }
}
