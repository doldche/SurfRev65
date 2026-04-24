#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout SurfRev65Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat>("reverb","Reverb", juce::NormalisableRange<float>(0.f,10.f,0.01f), 5.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>("tone",  "Tone",   juce::NormalisableRange<float>(0.f,10.f,0.01f), 5.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>("level", "Level",  juce::NormalisableRange<float>(0.f,10.f,0.01f), 7.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>("gain",  "Input Gain", juce::NormalisableRange<float>(0.5f,20.f,0.1f), 4.f));
    p.push_back (std::make_unique<juce::AudioParameterChoice>("mode", "Mode",   juce::StringArray{"Spring","Room","Hall"}, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool>("trem",   "Tremolo", false));
    p.push_back (std::make_unique<juce::AudioParameterBool>("boost",  "Boost +6dB", false));
    p.push_back (std::make_unique<juce::AudioParameterBool>("locut",  "Lo-Cut 180Hz", false));
    p.push_back (std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    return { p.begin(), p.end() };
}

SurfRev65Processor::SurfRev65Processor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Params", createLayout())
{
    for (auto id : {"reverb","tone","level","gain","mode","trem","boost","locut","bypass"})
        apvts.addParameterListener (id, this);
    pReverb = apvts.getRawParameterValue ("reverb");
    pTone   = apvts.getRawParameterValue ("tone");
    pLevel  = apvts.getRawParameterValue ("level");
    pGain   = apvts.getRawParameterValue ("gain");
    pMode   = apvts.getRawParameterValue ("mode");
    pTrem   = apvts.getRawParameterValue ("trem");
    pBoost  = apvts.getRawParameterValue ("boost");
    pLoCut  = apvts.getRawParameterValue ("locut");
    pBypass = apvts.getRawParameterValue ("bypass");
}

SurfRev65Processor::~SurfRev65Processor()
{
    for (auto id : {"reverb","tone","level","gain","mode","trem","boost","locut","bypass"})
        apvts.removeParameterListener (id, this);
}

void SurfRev65Processor::rebuildIR (Mode m, double sr)
{
    struct Res { float freq, amp, decay; };
    struct Cfg { float dur, envDecay; std::vector<Res> res; };
    const Cfg cfgs[3] = {
        { 3.8f, 2.5f, {{750,0.10f,5},{1100,0.07f,4},{380,0.05f,3},{550,0.04f,3.5f},{920,0.03f,4.5f}} },
        { 1.5f, 4.5f, {{400,0.06f,6},{900,0.04f,5},{250,0.03f,7}} },
        { 5.5f, 1.2f, {{300,0.08f,3},{600,0.05f,2.5f},{1200,0.04f,4},{180,0.06f,2}} }
    };
    const auto& c = cfgs[m];
    const int len = static_cast<int>(sr * c.dur);
    juce::AudioBuffer<float> ir (2, len);
    juce::Random rng;
    for (int ch = 0; ch < 2; ++ch)
    {
        float* d = ir.getWritePointer (ch);
        const float chOff = ch * 0.003f;
        for (int i = 0; i < len; ++i)
        {
            const float t   = static_cast<float>(i) / static_cast<float>(sr);
            const float env = std::exp (-t * c.envDecay);
            float s = (rng.nextFloat() * 2.f - 1.f) * env * 0.65f;
            for (auto& r : c.res)
                s += std::sin (juce::MathConstants<float>::twoPi * r.freq * (t + chOff)) * r.amp * std::exp (-t * r.decay);
            if (i < static_cast<int>(sr * 0.002))
                s *= static_cast<float>(i) / static_cast<float>(sr * 0.002);
            d[i] = s;
        }
    }
    convolver.loadImpulseResponse (std::move (ir), sr,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        juce::dsp::Convolution::Normalise::yes);
}

void SurfRev65Processor::prepareToPlay (double sr, int bs)
{
    currentSR = sr; currentBS = bs;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32)bs, 2 };
    convolver.prepare (spec);
    rebuildIR (currentMode, sr);
    auto tc = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 8000.0);
    auto lc = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 80.0);
    toneL.prepare (spec); toneL.coefficients = tc;
    toneR.prepare (spec); toneR.coefficients = tc;
    lcL.prepare (spec);   lcL.coefficients   = lc;
    lcR.prepare (spec);   lcR.coefficients   = lc;
    dry.setSize (2, bs); wet.setSize (2, bs);
    tremPhase = 0.f; needRebuild = false;
}

void SurfRev65Processor::releaseResources() {}

void SurfRev65Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (needRebuild) { rebuildIR (currentMode, currentSR); needRebuild = false; }
    const int N = buffer.getNumSamples();
    const int CH = buffer.getNumChannels();
    const bool bypass = pBypass->load() > 0.5f;
    if (bypass) return;
    const float rev   = pReverb->load() / 10.f;
    const float tone  = pTone->load()   / 10.f;
    const float level = pLevel->load()  / 10.f;
    const float gain  = pGain->load();
    const bool  trem  = pTrem->load()  > 0.5f;
    const bool  boost = pBoost->load() > 0.5f;
    const bool  locut = pLoCut->load() > 0.5f;
    buffer.applyGain (gain);
    if (locut)
    {
        auto lcc = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSR, 180.0);
        lcL.coefficients = lcc; lcR.coefficients = lcc;
        for (int i = 0; i < N; ++i)
        {
            if (CH > 0) buffer.getWritePointer(0)[i] = lcL.processSample (buffer.getReadPointer(0)[i]);
            if (CH > 1) buffer.getWritePointer(1)[i] = lcR.processSample (buffer.getReadPointer(1)[i]);
        }
    }
    const float toneFreq = 200.f + tone * 16000.f;
    auto tc = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSR, (double)toneFreq);
    toneL.coefficients = tc; toneR.coefficients = tc;
    dry.makeCopyOf (buffer);
    wet.makeCopyOf (buffer);
    for (int i = 0; i < N; ++i)
    {
        if (CH > 0) wet.getWritePointer(0)[i] = toneL.processSample (wet.getReadPointer(0)[i]);
        if (CH > 1) wet.getWritePointer(1)[i] = toneR.processSample (wet.getReadPointer(1)[i]);
    }
    juce::dsp::AudioBlock<float> wb (wet);
    convolver.process (juce::dsp::ProcessContextReplacing<float>(wb));
    const float dryG = 0.9f;
    const float wetG = rev * 2.2f;
    const float mast = level * (boost ? 2.0f : 1.2f);
    buffer.clear();
    for (int ch = 0; ch < CH; ++ch)
    {
        buffer.addFrom (ch, 0, dry, ch, 0, N, dryG);
        buffer.addFrom (ch, 0, wet, ch, 0, N, wetG);
    }
    if (trem)
    {
        const float rate = 5.0f / static_cast<float>(currentSR);
        for (int i = 0; i < N; ++i)
        {
            const float mod = 0.65f + 0.35f * std::sin (tremPhase * juce::MathConstants<float>::twoPi);
            tremPhase = std::fmod (tremPhase + rate, 1.0f);
            for (int ch = 0; ch < CH; ++ch)
                buffer.getWritePointer(ch)[i] *= mod;
        }
    }
    buffer.applyGain (mast);
    for (int ch = 0; ch < CH; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < N; ++i) d[i] = std::tanh (d[i]);
    }
}

void SurfRev65Processor::setCurrentProgram (int idx)
{
    currentProgram = idx;
    struct Pre { float rev,tone,lev,gain; int mode; bool trem,boost,locut; };
    const Pre pres[4] = {
        {6,5.5f,7,4,0,false,false,false},
        {4,6,7.5f,3,1,false,false,true},
        {8,4.5f,6.5f,3.5f,2,false,false,true},
        {5,6,7,4,0,true,false,false}
    };
    auto& p = pres[idx];
    auto set = [&](const char* id, float v) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1(v));
    };
    set("reverb",(float)p.rev); set("tone",(float)p.tone);
    set("level",(float)p.lev); set("gain",(float)p.gain);
    if (auto* m = apvts.getParameter("mode")) m->setValueNotifyingHost (p.mode / 2.f);
    auto setB = [&](const char* id, bool v){
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost (v ? 1.f : 0.f);
    };
    setB("trem",p.trem); setB("boost",p.boost); setB("locut",p.locut);
}

const juce::String SurfRev65Processor::getProgramName (int i)
{
    const char* names[] = {"Surf Clasico","Ballad Room","Big Hall","Tremolo Vintage"};
    return (i>=0 && i<4) ? names[i] : "";
}

void SurfRev65Processor::parameterChanged (const juce::String& id, float v)
{
    if (id == "mode") { currentMode = static_cast<Mode>(static_cast<int>(v)); needRebuild = true; }
}

void SurfRev65Processor::getStateInformation (juce::MemoryBlock& dest)
{
    auto xml = apvts.copyState().createXml();
    copyXmlToBinary (*xml, dest);
}

void SurfRev65Processor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SurfRev65Processor();
}
