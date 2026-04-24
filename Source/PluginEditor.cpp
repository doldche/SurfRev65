#include "PluginEditor.h"

SurfRev65Editor::SurfRev65Editor (SurfRev65Processor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (560, 680);
    setResizable (true, true);
    setResizeLimits (420, 540, 900, 1100);

    // Knobs originales
    attRev  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"reverb", knobReverb);
    attTone = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"tone",   knobTone);
    attLev  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"level",  knobLevel);
    attGain = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"gain",   knobGain);
    // Knobs nuevos
    attDwell   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"dwell",   knobDwell);
    attWetTone = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"wettone", knobWetTone);
    attMix     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts,"mix",     knobMix);

    addAndMakeVisible (knobReverb);
    addAndMakeVisible (knobTone);
    addAndMakeVisible (knobLevel);
    addAndMakeVisible (knobGain);
    addAndMakeVisible (knobDwell);
    addAndMakeVisible (knobWetTone);
    addAndMakeVisible (knobMix);

    attTrem   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"trem",   togTrem);
    attBoost  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"boost",  togBoost);
    attLoCut  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"locut",  togLoCut);
    attBypass = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,"bypass", togBypass);
    addAndMakeVisible (togTrem);
    addAndMakeVisible (togBoost);
    addAndMakeVisible (togLoCut);
    addAndMakeVisible (togBypass);

    modeCombo.addItemList ({"Fender 63","Room","Hall"}, 1);
    addChildComponent (modeCombo);
    attMode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,"mode",modeCombo);

    auto setupModeBtn = [&](juce::TextButton& btn, int idx)
    {
        addAndMakeVisible (btn);
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour(0xffA8C8D8));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour(0xff2A6080));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour(0xff1A3A50));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour(0xffF0F8FF));
        btn.onClick = [this, idx] { modeCombo.setSelectedItemIndex(idx, juce::sendNotificationAsync); updateModeButtons(); };
    };
    setupModeBtn (btnSpring, 0);
    setupModeBtn (btnRoom,   1);
    setupModeBtn (btnHall,   2);
    updateModeButtons();
    modeCombo.onChange = [this] { updateModeButtons(); };

    presetBox.addItem ("Fender 63 Classic",  1);
    presetBox.addItem ("Ballad Room",        2);
    presetBox.addItem ("Big Hall",           3);
    presetBox.addItem ("Tremolo Vintage",    4);
    presetBox.setSelectedItemIndex (0);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xffF0F8FF));
    presetBox.setColour (juce::ComboBox::textColourId,       juce::Colour(0xff1A3A50));
    presetBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour(0xff7AAABB));
    presetBox.onChange = [this] { proc.setCurrentProgram(presetBox.getSelectedItemIndex()); updateModeButtons(); };
    addAndMakeVisible (presetBox);

    startTimerHz (30);
}

SurfRev65Editor::~SurfRev65Editor() { stopTimer(); }

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
            b->getToggleState() ? juce::Colour(0xff2A6080) : juce::Colour(0xffA8C8D8));
}

void SurfRev65Editor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.setColour (juce::Colour (0xff2A5068));
    g.fillRoundedRectangle (bounds.toFloat(), 10.f);
    g.setColour (juce::Colour (0xffB0C8D8));
    g.drawRoundedRectangle (bounds.toFloat().reduced(1.f), 10.f, 2.f);

    auto body = bounds.reduced (14);
    paintPedalBody (g, body);
    paintScrews    (g, body);

    auto plateArea = body.removeFromTop (110).reduced (10);
    paintNamePlate (g, plateArea);

    paintLED (g, juce::Point<float>(bounds.getRight()-26.f, bounds.getY()+26.f));

    auto stompArea = juce::Rectangle<int>(bounds.getCentreX()-38, bounds.getBottom()-115, 76, 76);
    paintStomp (g, stompArea);

    auto vuArea = bounds.removeFromBottom (22).reduced (18, 3);
    paintVU (g, vuArea);

    g.setFont (juce::Font (8.f));
    g.setColour (juce::Colour (0xff7AAABB));
    g.drawText ("INPUT",  bounds.getX()+14, bounds.getBottom()-20, 40, 10, juce::Justification::centred);
    g.drawText ("OUTPUT", bounds.getRight()-54, bounds.getBottom()-20, 48, 10, juce::Justification::centred);

    const bool bp = proc.apvts.getRawParameterValue("bypass")->load() > 0.5f;
    g.drawText (bp ? "BYPASS ON" : "FX ON", bounds.getCentreX()-30, bounds.getBottom()-34, 60, 10, juce::Justification::centred);

    // Separador entre filas de knobs
    g.setColour (juce::Colour(0x30C0D8E8));
    g.drawHorizontalLine (bounds.getY() + 240, bounds.getX()+20.f, bounds.getRight()-20.f);

    // Labels de secciones
    g.setFont (juce::Font (8.f));
    g.setColour (juce::Colour (0xff7AAABB));
    g.drawText ("— INPUT —",    bounds.getX()+20,  bounds.getY()+128, 120, 10, juce::Justification::left);
    g.drawText ("— REVERB —",   bounds.getX()+20,  bounds.getY()+222, 120, 10, juce::Justification::left);
}

void SurfRev65Editor::paintPedalBody (juce::Graphics& g, juce::Rectangle<int> b)
{
    juce::ColourGradient grad(juce::Colour(0xffF5F0E8),b.getX(),b.getY(),
                               juce::Colour(0xffE0D8C8),b.getRight(),b.getBottom(),false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(b.toFloat(), 10.f);
    g.setColour(juce::Colour(0xffC0D0DC));
    g.drawRoundedRectangle(b.toFloat(), 10.f, 2.5f);
    g.setColour(juce::Colour(0x12000000));
    for(int i=0; i<b.getHeight(); i+=6)
        g.drawLine(b.getX(), b.getY()+i, b.getRight(), b.getY()+i, 0.5f);
}

void SurfRev65Editor::paintWoodGrain (juce::Graphics& g, juce::Rectangle<int> b)
{
    juce::ignoreUnused(g,b);
}

void SurfRev65Editor::paintNamePlate (juce::Graphics& g, juce::Rectangle<int> b)
{
    juce::ColourGradient grad(juce::Colour(0xff5BA8C8),b.getX(),b.getY(),
                               juce::Colour(0xff3A7A9A),b.getX(),b.getBottom(),false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(b.toFloat(), 8.f);
    g.setColour(juce::Colour(0xffC0D8E8));
    g.drawRoundedRectangle(b.toFloat(), 8.f, 2.f);
    g.drawRoundedRectangle(b.toFloat().reduced(5.f), 5.f, 1.f);
    g.setFont(juce::Font("Georgia", 40.f, juce::Font::bold));
    g.setColour(juce::Colour(0xffFFFFFF));
    g.drawText("SURF", b.reduced(8), juce::Justification::centredTop);
    g.setFont(juce::Font("Arial", 14.f, juce::Font::bold));
    g.setColour(juce::Colour(0xffE0F0FF));
    g.drawText("REV-65", b, juce::Justification::centred);
    g.setFont(juce::Font("Arial", 8.f, juce::Font::italic));
    g.setColour(juce::Colour(0xffB0D8F0));
    g.drawText("spring reverb  ·  hand-wired  ·  vintage", b.reduced(8), juce::Justification::centredBottom);
}

void SurfRev65Editor::paintLED (juce::Graphics& g, juce::Point<float> c)
{
    const bool bypass = proc.apvts.getRawParameterValue("bypass")->load() > 0.5f;
    g.setColour(juce::Colour(0xff1A3A50));
    g.fillEllipse(c.x-7, c.y-7, 14, 14);
    if(!bypass){
        g.setColour(juce::Colour(0xffFF3030));
        g.fillEllipse(c.x-5, c.y-5, 10, 10);
        g.setColour(juce::Colour(0x60FF3030));
        g.fillEllipse(c.x-10, c.y-10, 20, 20);
    } else {
        g.setColour(juce::Colour(0xff501010));
        g.fillEllipse(c.x-5, c.y-5, 10, 10);
    }
    g.setFont(juce::Font(7.f));
    g.setColour(juce::Colour(0xff7AAABB));
    g.drawText("ON", (int)c.x-10, (int)c.y+7, 20, 9, juce::Justification::centred);
}

void SurfRev65Editor::paintStomp (juce::Graphics& g, juce::Rectangle<int> b)
{
    auto bf=b.toFloat();
    float cx=bf.getCentreX(), cy=bf.getCentreY(), r=bf.getWidth()*.5f;
    g.setColour(juce::Colour(0x66000000));
    g.fillEllipse(cx-r+2, cy-r+4, r*2, r*2);
    juce::ColourGradient grad(juce::Colour(0xffD0D8E0),cx-r,cy-r,
                               juce::Colour(0xff8090A0),cx+r,cy+r,false);
    g.setGradientFill(grad);
    g.fillEllipse(cx-r, cy-r, r*2, r*2);
    g.setColour(juce::Colour(0xff90A8B8));
    g.drawEllipse(cx-r, cy-r, r*2, r*2, 3.f);
    g.setColour(juce::Colour(0x40FFFFFF));
    g.fillEllipse(cx-r*.6f, cy-r*.7f, r*.8f, r*.5f);
}

void SurfRev65Editor::paintScrews (juce::Graphics& g, juce::Rectangle<int> b)
{
    for(auto pt : {juce::Point<float>(b.getX()+10.f, b.getY()+10.f),
                   juce::Point<float>(b.getRight()-10.f, b.getY()+10.f),
                   juce::Point<float>(b.getX()+10.f, b.getBottom()-10.f),
                   juce::Point<float>(b.getRight()-10.f, b.getBottom()-10.f)})
    {
        juce::ColourGradient grad(juce::Colour(0xffE0E8F0),pt.x-5,pt.y-5,
                                   juce::Colour(0xff8898A8),pt.x+5,pt.y+5,false);
        g.setGradientFill(grad);
        g.fillEllipse(pt.x-5, pt.y-5, 10, 10);
        g.setColour(juce::Colour(0xff6080A0));
        g.drawEllipse(pt.x-5, pt.y-5, 10, 10, 1.f);
        g.drawLine(pt.x-3, pt.y, pt.x+3, pt.y, 1.f);
        g.drawLine(pt.x, pt.y-3, pt.x, pt.y+3, 1.f);
    }
}

void SurfRev65Editor::paintVU (juce::Graphics& g, juce::Rectangle<int> b)
{
    g.setColour(juce::Colour(0xff1A3A50));
    g.fillRoundedRectangle(b.toFloat(), 3.f);
    const int N=22;
    const float bw=b.getWidth()/(float)N;
    const int active=static_cast<int>(vuLevel*N);
    for(int i=0;i<N;i++){
        juce::Colour col;
        if(i<active)
            col = i<N*.6f ? juce::Colour(0xff40A8C0) :
                  i<N*.85f? juce::Colour(0xffF0C030) : juce::Colour(0xffFF4030);
        else
            col = i<N*.6f ? juce::Colour(0xff1A4858) :
                  i<N*.85f? juce::Colour(0xff3A3010) : juce::Colour(0xff3A1010);
        g.setColour(col);
        g.fillRoundedRectangle(b.getX()+i*bw+1, b.getY()+2, bw-2, b.getHeight()-4, 1.f);
    }
}

void SurfRev65Editor::resized()
{
    auto b = getLocalBounds().reduced(18);
    b.removeFromTop(120); // placa

    // FILA 1: Gain, Tone In, Level, Reverb
    auto row1 = b.removeFromTop(100);
    const int kw1 = row1.getWidth() / 4;
    knobGain  .setBounds(row1.removeFromLeft(kw1).reduced(3));
    knobTone  .setBounds(row1.removeFromLeft(kw1).reduced(3));
    knobLevel .setBounds(row1.removeFromLeft(kw1).reduced(3));
    knobReverb.setBounds(row1.reduced(3));

    b.removeFromTop(6);

    // FILA 2: Dwell, Tone Wet, Mix (los 3 nuevos)
    auto row2 = b.removeFromTop(100);
    const int kw2 = row2.getWidth() / 3;
    knobDwell  .setBounds(row2.removeFromLeft(kw2).reduced(3));
    knobWetTone.setBounds(row2.removeFromLeft(kw2).reduced(3));
    knobMix    .setBounds(row2.reduced(3));

    b.removeFromTop(8);

    // Mode buttons
    auto mRow = b.removeFromTop(26).reduced(4,0);
    const int mw = mRow.getWidth()/3;
    btnSpring.setBounds(mRow.removeFromLeft(mw).reduced(2));
    btnRoom  .setBounds(mRow.removeFromLeft(mw).reduced(2));
    btnHall  .setBounds(mRow.reduced(2));

    b.removeFromTop(6);

    // Presets
    presetBox.setBounds(b.removeFromTop(24).reduced(4,0));
    b.removeFromTop(8);

    // Toggles
    auto tRow = b.removeFromTop(24);
    const int tw = tRow.getWidth()/4;
    togTrem  .setBounds(tRow.removeFromLeft(tw).reduced(2));
    togBoost .setBounds(tRow.removeFromLeft(tw).reduced(2));
    togLoCut .setBounds(tRow.removeFromLeft(tw).reduced(2));
    togBypass.setBounds(tRow.reduced(2));
}

juce::AudioProcessorEditor* SurfRev65Processor::createEditor()
{
    return new SurfRev65Editor(*this);
}
