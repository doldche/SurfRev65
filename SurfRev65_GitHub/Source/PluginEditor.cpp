#include "PluginEditor.h"

//==============================================================================
SurfRev65Editor::SurfRev65Editor (SurfRev65Processor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (500, 640);
    setResizable (true, true);
    setResizeLimits (380, 500, 800, 1000);

    // Knob attachments
    attRev  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"reverb", knobReverb);
    attTone = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"tone",   knobTone);
    attLev  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"level",  knobLevel);
    attGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"gain",   knobGain);

    addAndMakeVisible (knobReverb);
    addAndMakeVisible (knobTone);
    addAndMakeVisible (knobLevel);
    addAndMakeVisible (knobGain);

    // Toggle attachments
    attTrem   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"trem",   togTrem);
    attBoost  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"boost",  togBoost);
    attLoCut  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"locut",  togLoCut);
    attBypass = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"bypass", togBypass);

    addAndMakeVisible (togTrem);
    addAndMakeVisible (togBoost);
    addAndMakeVisible (togLoCut);
    addAndMakeVisible (togBypass);

    // Mode: combo oculto para APVTS + botones visuales
    modeCombo.addItemList ({"Spring","Room","Hall"}, 1);
    addChildComponent (modeCombo);
    attMode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (p.apvts,"mode",modeCombo);

    auto setupModeBtn = [&](juce::TextButton& btn, int idx)
    {
        addAndMakeVisible (btn);
        btn.setColour (juce::TextButton::buttonColourId,    juce::Colour(0xffC8A86B));
        btn.setColour (juce::TextButton::buttonOnColourId,  juce::Colour(0xff5C3317));
        btn.setColour (juce::TextButton::textColourOffId,   juce::Colour(0xff2A1500));
        btn.setColour (juce::TextButton::textColourOnId,    juce::Colour(0xffF5EDD6));
        btn.onClick = [this, idx]
        {
            modeCombo.setSelectedItemIndex (idx, juce::sendNotificationAsync);
            updateModeButtons();
        };
    };
    setupModeBtn (btnSpring, 0);
    setupModeBtn (btnRoom,   1);
    setupModeBtn (btnHall,   2);
    updateModeButtons();

    modeCombo.onChange = [this] { updateModeButtons(); };

    // Presets
    presetBox.addItem ("Surf Clasico",    1);
    presetBox.addItem ("Ballad Room",     2);
    presetBox.addItem ("Big Hall",        3);
    presetBox.addItem ("Tremolo Vintage", 4);
    presetBox.setSelectedItemIndex (0);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xffF5EDD6));
    presetBox.setColour (juce::ComboBox::textColourId,       juce::Colour(0xff2A1500));
    presetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour(0xff8B6914));
    presetBox.onChange = [this]
    {
        proc.setCurrentProgram (presetBox.getSelectedItemIndex());
        updateModeButtons();
    };
    addAndMakeVisible (presetBox);

    startTimerHz (30);
}

SurfRev65Editor::~SurfRev65Editor() { stopTimer(); }

//==============================================================================
void SurfRev65Editor::timerCallback()
{
    const float rv = proc.apvts.getRawParameterValue("reverb")->load() / 10.f;
    vuLevel = vuLevel * 0.88f + rv * 0.12f;
    repaint();
}

void SurfRev65Editor::updateModeButtons()
{
    int cur = modeCombo.getSelectedItemIndex();
    btnSpring.setToggleState (cur==0, juce::dontSendNotification);
    btnRoom  .setToggleState (cur==1, juce::dontSendNotification);
    btnHall  .setToggleState (cur==2, juce::dontSendNotification);
    for (auto* b : {&btnSpring,&btnRoom,&btnHall})
        b->setColour (juce::TextButton::buttonColourId,
            b->getToggleState() ? juce::Colour(0xff5C3317) : juce::Colour(0xffC8A86B));
}

//==============================================================================
void SurfRev65Editor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Fondo oscuro
    g.setColour (juce::Colour (0xff1A0A04));
    g.fillRoundedRectangle (bounds.toFloat(), 10.f);

    // Cuerpo del pedal
    auto body = bounds.reduced (14);
    paintPedalBody (g, body);
    paintWoodGrain (g, body);
    paintScrews    (g, body);

    // Placa nombre (top)
    auto plateArea = body.removeFromTop (105).reduced (10);
    paintNamePlate (g, plateArea);

    // LED
    paintLED (g, juce::Point<float>(bounds.getRight()-26.f, bounds.getY()+26.f));

    // Stomp
    auto stompArea = juce::Rectangle<int>(bounds.getCentreX()-38, bounds.getBottom()-110, 76, 76);
    paintStomp (g, stompArea);

    // VU
    auto vuArea = bounds.removeFromBottom (22).reduced (18, 3);
    paintVU (g, vuArea);

    // Labels jacks
    g.setFont (juce::Font (8.f));
    g.setColour (juce::Colour (0xff8A6030));
    g.drawText ("INPUT",  bounds.getX()+14, bounds.getBottom()-20, 40, 10, juce::Justification::centred);
    g.drawText ("OUTPUT", bounds.getRight()-54, bounds.getBottom()-20, 48, 10, juce::Justification::centred);

    // "BYPASS" label bajo stomp
    g.setFont (juce::Font (8.f));
    g.setColour (juce::Colour (0xff8A6030));
    const bool bp = proc.apvts.getRawParameterValue("bypass")->load() > 0.5f;
    g.drawText (bp ? "BYPASS ON" : "FX ON",
                bounds.getCentreX()-30, bounds.getBottom()-32, 60, 10,
                juce::Justification::centred);
}

void SurfRev65Editor::paintPedalBody (juce::Graphics& g, juce::Rectangle<int> b)
{
    g.setColour (juce::Colour (0xff5C3317));
    g.fillRoundedRectangle (b.toFloat(), 10.f);
    g.setColour (juce::Colour (0xff3A2510));
    g.drawRoundedRectangle (b.toFloat(), 10.f, 2.5f);
}

void SurfRev65Editor::paintWoodGrain (juce::Graphics& g, juce::Rectangle<int> b)
{
    juce::Random rng(42);
    for (int i = 0; i < 20; ++i)
    {
        float y  = b.getY() + i*(b.getHeight()/20.f) + rng.nextFloat()*5.f;
        float dy = rng.nextFloat()*7.f - 3.5f;
        g.setColour (juce::Colour(i%2==0 ? 0x07FFD080 : 0x0E000000));
        g.drawLine ((float)b.getX(), y, (float)b.getRight(), y+dy, 1.5f);
    }
    g.setColour (juce::Colour(0x09000000));
    g.drawEllipse (b.getRight()-90.f, b.getY()+90.f, 42.f, 22.f, 1.f);
    g.drawEllipse (b.getRight()-80.f, b.getY()+96.f, 24.f, 12.f, 0.8f);
}

void SurfRev65Editor::paintNamePlate (juce::Graphics& g, juce::Rectangle<int> b)
{
    g.setColour (juce::Colour(0xffC8A86B));
    g.fillRoundedRectangle (b.toFloat(), 7.f);
    g.setColour (juce::Colour(0xff8B6914));
    g.drawRoundedRectangle (b.toFloat(), 7.f, 2.f);
    g.drawRoundedRectangle (b.toFloat().reduced(5.f), 4.f, 1.f);

    g.setFont (juce::Font ("Georgia", 38.f, juce::Font::bold));
    g.setColour (juce::Colour(0xff2A1500));
    g.drawText ("SURF", b.reduced(8), juce::Justification::centredTop);

    g.setFont (juce::Font ("Arial", 14.f, juce::Font::bold));
    g.setColour (juce::Colour(0xff5A3A08));
    g.drawText ("REV-65", b, juce::Justification::centred);

    g.setFont (juce::Font ("Arial", 8.f, juce::Font::italic));
    g.setColour (juce::Colour(0xff7A5010));
    g.drawText ("spring reverb  ·  hand-wired  ·  vintage",
                b.reduced(8), juce::Justification::centredBottom);
}

void SurfRev65Editor::paintLED (juce::Graphics& g, juce::Point<float> c)
{
    const bool bypass = proc.apvts.getRawParameterValue("bypass")->load() > 0.5f;
    g.setColour (juce::Colour(0xff1A0A04));
    g.fillEllipse (c.x-7, c.y-7, 14, 14);
    if (!bypass)
    {
        g.setColour (juce::Colour(0xffF5A030));
        g.fillEllipse (c.x-5, c.y-5, 10, 10);
        g.setColour (juce::Colour(0x50F5A030));
        g.fillEllipse (c.x-10, c.y-10, 20, 20);
    }
    else
    {
        g.setColour (juce::Colour(0xff3A1A00));
        g.fillEllipse (c.x-5, c.y-5, 10, 10);
    }
    g.setFont (juce::Font(7.f));
    g.setColour (juce::Colour(0xff8A6030));
    g.drawText ("ON", (int)c.x-10, (int)c.y+7, 20, 9, juce::Justification::centred);
}

void SurfRev65Editor::paintStomp (juce::Graphics& g, juce::Rectangle<int> b)
{
    auto bf = b.toFloat();
    float cx=bf.getCentreX(), cy=bf.getCentreY(), r=bf.getWidth()*.5f;
    g.setColour (juce::Colour(0x66000000));
    g.fillEllipse (cx-r+2, cy-r+4, r*2, r*2);
    g.setColour (juce::Colour(0xff1A0E06));
    g.fillEllipse (cx-r, cy-r, r*2, r*2);
    g.setColour (juce::Colour(0xff4A3020));
    g.drawEllipse (cx-r, cy-r, r*2, r*2, 3.f);
    g.setColour (juce::Colour(0x12D4A040));
    g.drawEllipse (cx-r*.7f, cy-r*.7f, r*1.4f, r*1.4f, 0.8f);
}

void SurfRev65Editor::paintScrews (juce::Graphics& g, juce::Rectangle<int> b)
{
    for (auto pt : {juce::Point<float>(b.getX()+10.f,  b.getY()+10.f),
                    juce::Point<float>(b.getRight()-10.f, b.getY()+10.f),
                    juce::Point<float>(b.getX()+10.f,  b.getBottom()-10.f),
                    juce::Point<float>(b.getRight()-10.f, b.getBottom()-10.f)})
    {
        g.setColour (juce::Colour(0xff9A8060));
        g.fillEllipse (pt.x-5, pt.y-5, 10, 10);
        g.setColour (juce::Colour(0xff3A2810));
        g.drawEllipse (pt.x-5, pt.y-5, 10, 10, 1.f);
        g.drawLine (pt.x-3, pt.y, pt.x+3, pt.y, 1.f);
        g.drawLine (pt.x, pt.y-3, pt.x, pt.y+3, 1.f);
    }
}

void SurfRev65Editor::paintVU (juce::Graphics& g, juce::Rectangle<int> b)
{
    g.setColour (juce::Colour(0xff1A0A04));
    g.fillRoundedRectangle (b.toFloat(), 3.f);
    const int N = 22;
    const float bw = b.getWidth() / (float)N;
    const int active = static_cast<int>(vuLevel * N);
    for (int i = 0; i < N; ++i)
    {
        juce::Colour col;
        if (i < active)
            col = i < N*.6f ? juce::Colour(0xff4A8A2A) :
                  i < N*.85f? juce::Colour(0xffD4A030) : juce::Colour(0xffCC3010);
        else
            col = i < N*.6f ? juce::Colour(0xff1A2A0A) :
                  i < N*.85f? juce::Colour(0xff2A1A00) : juce::Colour(0xff2A0A04);

        g.setColour (col);
        g.fillRoundedRectangle (b.getX()+i*bw+1, b.getY()+2, bw-2, b.getHeight()-4, 1.f);
    }
}

//==============================================================================
void SurfRev65Editor::resized()
{
    auto b = getLocalBounds().reduced (18);
    b.removeFromTop (115); // placa nombre

    // Knobs — 4 en fila
    auto kRow = b.removeFromTop (95);
    const int kw = kRow.getWidth() / 4;
    knobReverb.setBounds (kRow.removeFromLeft(kw).reduced(3));
    knobTone  .setBounds (kRow.removeFromLeft(kw).reduced(3));
    knobLevel .setBounds (kRow.removeFromLeft(kw).reduced(3));
    knobGain  .setBounds (kRow.reduced(3));

    b.removeFromTop (8);

    // Mode buttons
    auto mRow = b.removeFromTop (26).reduced(4,0);
    const int mw = mRow.getWidth()/3;
    btnSpring.setBounds (mRow.removeFromLeft(mw).reduced(2));
    btnRoom  .setBounds (mRow.removeFromLeft(mw).reduced(2));
    btnHall  .setBounds (mRow.reduced(2));

    b.removeFromTop (6);

    // Presets
    presetBox.setBounds (b.removeFromTop(24).reduced(4,0));

    b.removeFromTop (8);

    // Toggles — 4 en fila
    auto tRow = b.removeFromTop (24);
    const int tw = tRow.getWidth()/4;
    togTrem  .setBounds (tRow.removeFromLeft(tw).reduced(2));
    togBoost .setBounds (tRow.removeFromLeft(tw).reduced(2));
    togLoCut .setBounds (tRow.removeFromLeft(tw).reduced(2));
    togBypass.setBounds (tRow.reduced(2));

    // VU y stomp quedan pintados en paint()
}

//==============================================================================
juce::AudioProcessorEditor* SurfRev65Processor::createEditor()
{
    return new SurfRev65Editor (*this);
}
