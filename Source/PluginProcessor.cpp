#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SurfRev65Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>("reverb","Reverb",juce::NormalisableRange<float>(0.f,10.f,0.01f),5.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("tone","Tone",juce::NormalisableRange<float>(0.f,10.f,0.01f),5.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("level","Level",juce::NormalisableRange<float>(0.f,10.f,0.01f),7.f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("gain","Input Gain",juce::NormalisableRange<float>(0.5f,20.f,0.1f),4.f));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("mode","Mode",juce::StringArray{"Fender 63","Room","Hall"},0));
    p.push_back(std::make_unique<juce::AudioParameterBool>("trem","Tremolo",false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("boost","Boost +6dB",false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("locut","Lo-Cut 180Hz",false));
    p.push_back(std::make_unique<juce::AudioParameterBool>("bypass","Bypass",false));
    return {p.begin(),p.end()};
}

SurfRev65Processor::SurfRev65Processor()
    : AudioProcessor(BusesProperties()
          .withInput("Input",juce::AudioChannelSet::stereo(),true)
          .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
      apvts(*this,nullptr,"Params",createLayout())
{
    for(auto id:{"reverb","tone","level","gain","mode","trem","boost","locut","bypass"})
        apvts.addParameterListener(id,this);
    pReverb=apvts.getRawParameterValue("reverb");
    pTone=apvts.getRawParameterValue("tone");
    pLevel=apvts.getRawParameterValue("level");
    pGain=apvts.getRawParameterValue("gain");
    pMode=apvts.getRawParameterValue("mode");
    pTrem=apvts.getRawParameterValue("trem");
    pBoost=apvts.getRawParameterValue("boost");
    pLoCut=apvts.getRawParameterValue("locut");
    pBypass=apvts.getRawParameterValue("bypass");
}

SurfRev65Processor::~SurfRev65Processor()
{
    for(auto id:{"reverb","tone","level","gain","mode","trem","boost","locut","bypass"})
        apvts.removeParameterListener(id,this);
}

void SurfRev65Processor::rebuildIR(Mode m, double sr)
{
    if(m==Spring)
    {
        struct SpringDef {
            float preDelay,decay,f1,f2,f3,a1,a2,a3,flutterRate,flutterDepth;
        };
        const SpringDef springs[3] = {
            {0.008f,1.6f,820.f,1640.f,2460.f,0.13f,0.09f,0.05f,3.2f,0.004f},
            {0.011f,2.0f,750.f,1500.f,2250.f,0.11f,0.07f,0.04f,4.1f,0.003f},
            {0.014f,2.4f,680.f,1360.f,2040.f,0.09f,0.06f,0.03f,2.8f,0.005f},
        };
        const float duration=3.2f;
        const int len=static_cast<int>(sr*duration);
        juce::AudioBuffer<float> ir(2,len);
        juce::Random rng;
        for(int ch=0;ch<2;ch++)
        {
            float* d=ir.getWritePointer(ch);
            const float chOff=ch*0.007f;
            for(int i=0;i<len;i++)
            {
                const float t=static_cast<float>(i)/static_cast<float>(sr);
                float s=0.f;
                for(auto& sp:springs)
                {
                    if(t<sp.preDelay) continue;
                    const float tr=t-sp.preDelay;
                    const float env=std::exp(-tr*sp.decay);
                    const float flutter=1.f+sp.flutterDepth*
                        std::sin(juce::MathConstants<float>::twoPi*sp.flutterRate*tr);
                    s+=std::sin(juce::MathConstants<float>::twoPi*sp.f1*flutter*(tr+chOff))*sp.a1*env;
                    s+=std::sin(juce::MathConstants<float>::twoPi*sp.f2*flutter*(tr+chOff))*sp.a2*std::exp(-tr*sp.decay*1.3f);
                    s+=std::sin(juce::MathConstants<float>::twoPi*sp.f3*flutter*(tr+chOff))*sp.a3*std::exp(-tr*sp.decay*1.8f);
                    s+=(rng.nextFloat()*2.f-1.f)*env*0.55f;
                }
                if(i<static_cast<int>(sr*0.003))
                    s*=static_cast<float>(i)/(sr*0.003f);
                d[i]=s*(1.f-0.15f*std::min(t/duration,1.f));
            }
        }
        convolver.loadImpulseResponse(std::move(ir),sr,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::no,
            juce::dsp::Convolution::Normalise::yes);
    }
    else if(m==Room)
    {
        const int len=static_cast<int>(sr*1.5f);
        juce::AudioBuffer<float> ir(2,len);
        juce::Random rng;
        for(int ch=0;ch<2;ch++){
            float* d=ir.getWritePointer(ch);
            const float co=ch*0.003f;
            for(int i=0;i<len;i++){
                const float t=static_cast<float>(i)/static_cast<float>(sr);
                const float env=std::exp(-t*4.5f);
                float s=(rng.nextFloat()*2.f-1.f)*env*0.65f;
                s+=std::sin(6.283f*400.f*(t+co))*0.06f*std::exp(-t*6.f);
                s+=std::sin(6.283f*900.f*(t+co))*0.04f*std::exp(-t*5.f);
                d[i]=s;
            }
        }
        convolver.loadImpulseResponse(std::move(ir),sr,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::no,
            juce::dsp::Convolution::Normalise::yes);
    }
    else
    {
        const int len=static_cast<int>(sr*5.5f);
        juce::AudioBuffer<float> ir(2,len);
        juce::Random rng;
        for(int ch=0;ch<2;ch++){
            float* d=ir.getWritePointer(ch);
            const float co=ch*0.003f;
            for(int i=0;i<len;i++){
                const float t=static_cast<float>(i)/static_cast<float>(sr);
                const float env=std::exp(-t*1.2f);
                float s=(rng.nextFloat()*2.f-1.f)*env*0.65f;
                s+=std::sin(6.283f*300.f*(t+co))*0.08f*std::exp(-t*3.f);
                s+=std::sin(6.283f*600.f*(t+co))*0.05f*std::exp(-t*2.5f);
                s+=std::sin(6.283f*1200.f*(t+co))*0.04f*std::exp(-t*4.f);
                if(i<static_cast<int>(sr*0.002)) s*=static_cast<float>(i)/(sr*0.002f);
                d[i]=s;
            }
        }
        convolver.loadImpulseResponse(std::move(ir),sr,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::no,
            juce::dsp::Convolution::Normalise::yes);
    }
}

void SurfRev65Processor::prepareToPlay(double sr, int bs)
{
    currentSR=sr; currentBS=bs;
    juce::dsp::ProcessSpec spec{sr,(juce::uint32)bs,2};
    convolver.prepare(spec);
    rebuildIR(currentMode,sr);
    auto tc=juce::dsp::IIR::Coefficients<float>::makeLowPass(sr,8000.0);
    auto lc=juce::dsp::IIR::Coefficients<float>::makeHighPass(sr,80.0);
    toneL.prepare(spec); toneL.coefficients=tc;
    toneR.prepare(spec); toneR.coefficients=tc;
    lcL.prepare(spec); lcL.coefficients=lc;
    lcR.prepare(spec); lcR.coefficients=lc;
    dry.setSize(2,bs); wet.setSize(2,bs);
    tremPhase=0.f; needRebuild=false;
}

void SurfRev65Processor::releaseResources(){}

void SurfRev65Processor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if(needRebuild){rebuildIR(currentMode,currentSR); needRebuild=false;}
    const int N=buffer.getNumSamples(), CH=buffer.getNumChannels();
    const bool bypass=pBypass->load()>0.5f;
    if(bypass) return;
    const float rev=pReverb->load()/10.f;
    const float tone=pTone->load()/10.f;
    const float level=pLevel->load()/10.f;
    const float gain=pGain->load();
    const bool trem=pTrem->load()>0.5f;
    const bool boost=pBoost->load()>0.5f;
    const bool locut=pLoCut->load()>0.5f;

    buffer.applyGain(gain);

    if(currentMode==Spring)
    {
        auto bassBoost=juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            currentSR,250.0,1.0f,1.4f);
        lcL.coefficients=bassBoost; lcR.coefficients=bassBoost;
        for(int i=0;i<N;i++){
            if(CH>0) buffer.getWritePointer(0)[i]=lcL.processSample(buffer.getReadPointer(0)[i]);
            if(CH>1) buffer.getWritePointer(1)[i]=lcR.processSample(buffer.getReadPointer(1)[i]);
        }
    }
    else if(locut)
    {
        auto lcc=juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSR,180.0);
        lcL.coefficients=lcc; lcR.coefficients=lcc;
        for(int i=0;i<N;i++){
            if(CH>0) buffer.getWritePointer(0)[i]=lcL.processSample(buffer.getReadPointer(0)[i]);
            if(CH>1) buffer.getWritePointer(1)[i]=lcR.processSample(buffer.getReadPointer(1)[i]);
        }
    }

    const float toneFreq=currentMode==Spring ? 800.f+tone*6000.f : 200.f+tone*16000.f;
    auto tc=juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSR,(double)toneFreq);
    toneL.coefficients=tc; toneR.coefficients=tc;

    dry.makeCopyOf(buffer);
    wet.makeCopyOf(buffer);
    for(int i=0;i<N;i++){
        if(CH>0) wet.getWritePointer(0)[i]=toneL.processSample(wet.getReadPointer(0)[i]);
        if(CH>1) wet.getWritePointer(1)[i]=toneR.processSample(wet.getReadPointer(1)[i]);
    }

    juce::dsp::AudioBlock<float> wb(wet);
    convolver.process(juce::dsp::ProcessContextReplacing<float>(wb));

    const float dryG=0.9f;
    const float wetG=currentMode==Spring ? rev*2.8f : rev*2.2f;
    const float mast=level*(boost?2.0f:1.2f);

    buffer.clear();
    for(int ch=0;ch<CH;ch++){
        buffer.addFrom(ch,0,dry,ch,0,N,dryG);
        buffer.addFrom(ch,0,wet,ch,0,N,wetG);
    }

    if(trem){
        const float rate=5.0f/static_cast<float>(currentSR);
        for(int i=0;i<N;i++){
            const float mod=0.65f+0.35f*std::sin(tremPhase*juce::MathConstants<float>::twoPi);
            tremPhase=std::fmod(tremPhase+rate,1.0f);
            for(int ch=0;ch<CH;ch++) buffer.getWritePointer(ch)[i]*=mod;
        }
    }

    buffer.applyGain(mast);
    for(int ch=0;ch<CH;ch++){
        auto* d=buffer.getWritePointer(ch);
        for(int i=0;i<N;i++) d[i]=std::tanh(d[i]);
    }
}

void SurfRev65Processor::setCurrentProgram(int idx)
{
    currentProgram=idx;
    struct Pre{float rev,tone,lev,gain;int mode;bool trem,boost,locut;};
    const Pre pres[4]={
        {7,6,7,4,  0,false,false,false},
        {4,6,7.5f,3,1,false,false,true},
        {8,4.5f,6.5f,3.5f,2,false,false,true},
        {6,7,7,4,  0,true,false,false}
    };
    auto& p=pres[idx];
    auto set=[&](const char* id,float v){
        if(auto* param=apvts.getParameter(id))
            param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(v));
    };
    set("reverb",(float)p.rev); set("tone",(float)p.tone);
    set("level",(float)p.lev); set("gain",(float)p.gain);
    if(auto* m=apvts.getParameter("mode")) m->setValueNotifyingHost(p.mode/2.f);
    auto setB=[&](const char* id,bool v){
        if(auto* param=apvts.getParameter(id))
            param->setValueNotifyingHost(v?1.f:0.f);
    };
    setB("trem",p.trem); setB("boost",p.boost); setB("locut",p.locut);
}

const juce::String SurfRev65Processor::getProgramName(int i)
{
    const char* names[]={"Fender 63 Classic","Ballad Room","Big Hall","Tremolo Vintage"};
    return(i>=0&&i<4)?names[i]:"";
}

void SurfRev65Processor::parameterChanged(const juce::String& id,float v)
{
    if(id=="mode"){currentMode=static_cast<Mode>(static_cast<int>(v)); needRebuild=true;}
}

void SurfRev65Processor::getStateInformation(juce::MemoryBlock& dest)
{
    auto xml=apvts.copyState().createXml();
    copyXmlToBinary(*xml,dest);
}

void SurfRev65Processor::setStateInformation(const void* data,int size)
{
    auto xml=getXmlFromBinary(data,size);
    if(xml&&xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SurfRev65Processor();
}
