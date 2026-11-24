#include "PluginEditor.h"
#include <algorithm>

FartexAudioProcessorEditor::FartexAudioProcessorEditor (FartexAudioProcessor& p)
    : AudioProcessorEditor (&p), fartProcessor (p)
{
    setupKnob (driveKnob,  "DRIVE",    "drive");
    setupKnob (toneKnob,   "TONE",     "tone");
    setupKnob (mixKnob,    "MIX",      "mix");
    setupKnob (outputKnob, "OUTPUT",   "output");
    setupKnob (fartKnob,   "FART LVL", "fartlevel");
    setupKnob (trigKnob,   "TRIGGER",  "trigger");

    setSize (480, 610);
    startTimerHz (30);
}

FartexAudioProcessorEditor::~FartexAudioProcessorEditor()
{
    stopTimer();
}

void FartexAudioProcessorEditor::setupKnob (Knob& knob, const juce::String& labelText, const juce::String& paramId)
{
    addAndMakeVisible (knob.slider);
    knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    knob.slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff9be564));
    knob.slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a2f52));
    knob.slider.setColour (juce::Slider::thumbColourId, juce::Colours::white);
    knob.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    knob.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        fartProcessor.apvts, paramId, knob.slider);

    addAndMakeVisible (knob.label);
    knob.label.setText (labelText, juce::dontSendNotification);
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, juce::Colour (0xffcbbfe6));
    knob.label.setFont (juce::Font (juce::FontOptions (14.0f)));
}

void FartexAudioProcessorEditor::resized()
{
    buttArea = juce::Rectangle<int> (30, 75, getWidth() - 60, 270);
    buttCentre = juce::Point<float> ((float) buttArea.getCentreX(), (float) buttArea.getCentreY() + 5.0f);

    Knob* knobs[] = { &driveKnob, &toneKnob, &mixKnob, &outputKnob, &fartKnob, &trigKnob };

    const int cellW  = getWidth() / 3;
    const int cellH  = 125;
    const int startY = 355;

    for (int i = 0; i < 6; ++i)
    {
        auto cell = juce::Rectangle<int> ((i % 3) * cellW, startY + (i / 3) * cellH, cellW, cellH);
        knobs[i]->label.setBounds (cell.removeFromTop (20));
        knobs[i]->slider.setBounds (cell.reduced (12, 0));
    }
}

void FartexAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (buttArea.contains (e.getPosition()))
        fartProcessor.requestFart();
}

void FartexAudioProcessorEditor::timerCallback()
{
    const float raw = fartProcessor.fartActivity.load();
    const float driveDb = fartProcessor.apvts.getRawParameterValue ("drive")->load();

    aggression = juce::jlimit (0.0f, 1.0f, driveDb / 40.0f);
    activity   = juce::jmax (juce::jlimit (0.0f, 1.0f, raw * 1.6f), activity * 0.85f);

    // Jiggle: the harder it goes, the more the cheeks shake
    const float shakeAmount = activity * (2.0f + 7.0f * aggression);
    shakeL = (random.nextFloat() - 0.5f) * 2.0f * shakeAmount;
    shakeR = (random.nextFloat() - 0.5f) * 2.0f * shakeAmount;
    squish = activity * random.nextFloat();

    // Spawn stink clouds
    if (activity > 0.05f && puffs.size() < 150)
    {
        const int count = 1 + (int) (aggression * 3.0f + activity * 2.0f);

        for (int i = 0; i < count; ++i)
        {
            const float spread = 2.0f + 6.0f * aggression;
            puffs.push_back (Puff { buttCentre.x + (random.nextFloat() - 0.5f) * 24.0f,
                                    buttCentre.y + 60.0f,
                                    (random.nextFloat() - 0.5f) * spread,
                                    0.8f + random.nextFloat() * (1.0f + 3.0f * aggression),
                                    10.0f + random.nextFloat() * 14.0f,
                                    1.0f });
        }
    }

    // Move and fade the clouds
    for (auto& p : puffs)
    {
        p.x += p.vx;
        p.y += p.vy;
        p.vx *= 0.98f;
        p.size += 1.2f;
        p.life -= 0.025f;
    }

    puffs.erase (std::remove_if (puffs.begin(), puffs.end(),
                                 [] (const Puff& p) { return p.life <= 0.0f; }),
                 puffs.end());

    repaint();
}

void FartexAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2040), 0.0f, 0.0f,
                                             juce::Colour (0xff120e1c), 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Title
    g.setColour (juce::Colour (0xff9be564));
    g.setFont (juce::Font (juce::FontOptions (38.0f).withStyle ("Bold")));
    g.drawText ("FARTEX", 0, 8, getWidth(), 44, juce::Justification::centred);

    g.setColour (juce::Colour (0xff8f82ad));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("click the butt. turn up the drive. regret it.", 0, 50, getWidth(), 18, juce::Justification::centred);

    // The butt
    const float cx = buttCentre.x;
    const float cy = buttCentre.y;

    const auto skin  = juce::Colour (0xfff6c1a6).interpolatedWith (juce::Colour (0xffe0452f), aggression * 0.85f);
    const auto shade = skin.darker (0.45f);

    const float cheekW = 190.0f;
    const float cheekH = 190.0f + squish * 8.0f;

    const auto leftCheek  = juce::Rectangle<float> (cheekW, cheekH).withCentre (juce::Point<float> (cx - 88.0f + shakeL, cy));
    const auto rightCheek = juce::Rectangle<float> (cheekW, cheekH).withCentre (juce::Point<float> (cx + 88.0f + shakeR, cy));

    for (const auto& r : { leftCheek, rightCheek })
    {
        g.setColour (skin);
        g.fillEllipse (r);
        g.setColour (shade);
        g.drawEllipse (r, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillEllipse (r.getX() + 35.0f, r.getY() + 28.0f, 34.0f, 56.0f);
    }

    // The crack
    juce::Path crack;
    crack.startNewSubPath (cx, cy - 92.0f);
    crack.quadraticTo (cx + 14.0f, cy - 10.0f, cx, cy + 36.0f);
    g.setColour (shade.darker (0.4f));
    g.strokePath (crack, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // The business end
    const float holeR = 7.0f + activity * 12.0f;
    const float hx = cx;
    const float hy = cy + 52.0f;

    g.setColour (shade);
    for (int k = 0; k < 10; ++k)
    {
        const float ang = (float) k * juce::MathConstants<float>::twoPi / 10.0f;
        const float inner = holeR + 1.0f;
        const float outer = holeR + 9.0f + 5.0f * activity;
        g.drawLine (hx + std::cos (ang) * inner, hy + std::sin (ang) * inner,
                    hx + std::cos (ang) * outer, hy + std::sin (ang) * outer, 2.0f);
    }

    g.setColour (juce::Colour (0xff4a1414));
    g.fillEllipse (hx - holeR, hy - holeR * 0.8f, holeR * 2.0f, holeR * 1.6f);

    // Stink clouds
    for (const auto& p : puffs)
    {
        g.setColour (juce::Colour (0xff9bd04a).withAlpha (juce::jlimit (0.0f, 1.0f, p.life) * 0.45f));
        g.fillEllipse (p.x - p.size * 0.5f, p.y - p.size * 0.5f, p.size, p.size);
    }

    // Sound effect text
    if (activity > 0.15f)
    {
        const char* word = aggression < 0.33f ? "pfft" : (aggression < 0.66f ? "brrrp!" : "BRAAAAP!!");
        g.setColour (juce::Colour (0xff9be564).withAlpha (juce::jlimit (0.0f, 1.0f, activity)));
        g.setFont (juce::Font (juce::FontOptions (20.0f + 16.0f * aggression).withStyle ("Bold")));
        g.drawText (word, 0, 78, getWidth(), 34, juce::Justification::centred);
    }
}
