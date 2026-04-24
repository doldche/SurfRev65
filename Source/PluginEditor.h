#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class VintageKnob : public juce::Slider
{
public:
    VintageKnob (const juce::String& lbl) : juce::Slider(), label(lbl)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f, true);
        setVelocityBasedMode (false);
        setScrollWheelEnabled (true);
    }
    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (2.f);
        float cx = b.getCentreX(), cy = b.getCentreY() - 8.f;
        float r  = juce::jmin (b.getWidth(), b.getHeight() - 16.f) * 0.46f;
        g.setColour (juce::Colour (0x55000000));
        g.fillEllipse (cx-r+2, cy-r+4, r*2, r*2);
        g.setColour (juce::Colour (0xff2A1A08));
        g.fillEllipse (cx-r, cy-r, r*2, r*2);
        g.setColour (juce::Colour (0xff6B4820));
        g.drawEllipse (cx-r, cy-r, r*2, r*2, 2.5f);
        g.setColour (juce::Colour (0x30D4A040));
        g.drawEllipse (cx-r*.78f, cy-r*.78f, r*1.56f, r*1.56f, 0.8f);
        const float sa = juce::MathConstants<float>::pi * 1.25f;
        const float ea = juce::MathConstants<float>::pi * 2.75f;
        juce::Path arcBg;
        arcBg.addArc (cx-r*.88f, cy-r*.88f, r*1.76f, r*1.76f, sa, ea, true);
        g.setColour (juce::Colour (0x20D4A040));
        g.strokePath (arcBg, juce::PathStrokeType (1.5f));
        const float norm = static_cast<float>((getValue()-getMinimum())/(getMaximum()-getMinimum()));
        const float va = sa + norm * (ea - sa);
        juce::Path arcVal;
        arcVal.addArc (cx-r*.88f, cy-r*.88f, r*1.76f, r*1.76f, sa, va, true);
        g.setColour (juce::Colour (0xffD4A040));
        g.strokePath (arcVal, juce::PathStrokeType (2.0f));
        float mx = cx + std::sin(va)*r*0.56f, my = cy - std::cos(va)*r*0.56f;
        float sx = cx + std::sin(va)*r*0.12f, sy = cy - std::cos(va)*r*0.12f;
        g.drawLine (sx, sy, mx, my, 2.5f);
        g.setColour (juce::Colour (0xffD4B07A));
        g.setFont (juce::Font (9.f));
        g.drawText (label, getLocalBounds().removeFromBottom (14), juce::Justification::centred);
        g.setColour (juce::Colour (0xffA07840));
        g.setFont (juce::Font (8.f));
        g.drawText (juce::String (getValue(), 1),
                    getLocalBounds().removeFromBottom (26).removeFromTop(12),
                    juce::Justification::centred);
    }
private:
    juce::String label;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VintageKnob)
};

class VintageToggle : public juce::ToggleButton
{
public:
    VintageToggle (const juce::String& t) : juce::ToggleButton (t) {}
    void paintButton (juce::Graphics& g, bool hi, bool) override
    {
        auto b  = getLocalBounds().toFloat().reduced (1.f);
        bool on = getToggleState();
        g.setColour (on ? juce::Colour(0xff3A5A1A) : juce::Colour(0xff5C3317));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (on ? juce::Colour(0xff6A9A30) : juce::Colour(0xff8B6914));
        g.drawRoundedRectangle (b, 4.f, hi ? 1.5f : 1.f);
        g.setColour (on ? juce::Colour(0xffC8E890) : juce::Colour(0xffD4B07A));
        g.setFont (juce::Font (9.f, juce::Font::bold));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred);
    }
};

class SurfRev65Editor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    SurfRev65Editor (SurfRev65Processor&);
    ~SurfRev65Editor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
private:
    SurfRev65Processor& proc;
    VintageKnob knobReverb { "REVERB" };
    VintageKnob knobTone   { "TONE"   };
    VintageKnob knobLevel  { "LEVEL"  };
    VintageKnob knobGain   { "GAIN"   };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        attRev, attTone, attLev, attGain;
    VintageToggle togTrem   { "TREMOLO"  };
    VintageToggle togBoost  { "BOOST +6" };
    VintageToggle togLoCut  { "LO-CUT"  };
    VintageToggle togBypass { "BYPASS"   };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
        attTrem, attBoost, attLoCut, attBypass;
    juce::TextButton btnSpring { "Spring" };
    juce::TextButton btnRoom   { "Room"   };
    juce::TextButton btnHall   { "Hall"   };
    juce::ComboBox   modeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attMode;
    juce::ComboBox presetBox;
    float vuLevel = 0.f;
    void paintPedalBody  (juce::Graphics&, juce::Rectangle<int>);
    void paintWoodGrain  (juce::Graphics&, juce::Rectangle<int>);
    void paintNamePlate  (juce::Graphics&, juce::Rectangle<int>);
    void paintLED        (juce::Graphics&, juce::Point<float>);
    void paintStomp      (juce::Graphics&, juce::Rectangle<int>);
    void paintVU         (juce::Graphics&, juce::Rectangle<int>);
    void paintScrews     (juce::Graphics&, juce::Rectangle<int>);
    void updateModeButtons();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SurfRev65Editor)
};
