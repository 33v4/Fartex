#pragma once

#include "PluginProcessor.h"

class FartexAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit FartexAudioProcessorEditor (FartexAudioProcessor&);
    ~FartexAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    struct Puff
    {
        float x, y, vx, vy, size, life;
    };

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (Knob& knob, const juce::String& labelText, const juce::String& paramId);

    FartexAudioProcessor& fartProcessor;

    Knob driveKnob, toneKnob, mixKnob, outputKnob, fartKnob, trigKnob;

    juce::Rectangle<int> buttArea;
    juce::Point<float> buttCentre;

    std::vector<Puff> puffs;
    juce::Random random;

    float activity = 0.0f;     // how hard the butt is currently going
    float aggression = 0.0f;   // drive amount, 0..1
    float shakeL = 0.0f, shakeR = 0.0f, squish = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FartexAudioProcessorEditor)
};
