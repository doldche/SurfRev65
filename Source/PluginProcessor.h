#pragma once
#include <JuceHeader.h>

class SurfRev65Processor : public juce::AudioProcessor,
                            public juce::AudioProcessorValueTreeState::Listener
{
public:
    SurfRev65Processor();
    ~SurfRev65Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SURF REV-65"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int  getNumPrograms() override { return 4; }
    int  getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void parameterChanged (const juce::String& id, float val) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    enum Mode { Spring=0, Room=1, Hall=2 };

private:
    void rebuildIR (Mode m, double sr);

    juce::dsp::Convolution   convolver;
    juce::dsp::IIR::Filter<float> toneL, toneR, lcL, lcR;
    juce::AudioBuffer<float> dry, wet;

    double   currentSR   = 44100.0;
    int      currentBS   = 512;
    int      currentProgram = 0;
    Mode     currentMode = Spring;
    bool     needRebuild = false;

    std::atomic<float>* pReverb  = nullptr;
    std::atomic<float>* pTone    = nullptr;
    std::atomic<float>* pLevel   = nullptr;
    std::atomic<float>* pGain    = nullptr;
    std::atomic<float>* pMode    = nullptr;
    std::atomic<float>* pTrem    = nullptr;
    std::atomic<float>* pBoost   = nullptr;
    std::atomic<float>* pLoCut   = nullptr;
    std::atomic<float>* pBypass  = nullptr;

    float tremPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SurfRev65Processor)
};
