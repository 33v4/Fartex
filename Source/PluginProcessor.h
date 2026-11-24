#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "FartVoice.h"

class FartexAudioProcessor : public juce::AudioProcessor
{
public:
    FartexAudioProcessor();
    ~FartexAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Fartex"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Called by the editor when you click the butt
    void requestFart() { manualTrigger.store (true); }

    // Read by the editor to animate the butt (0..1ish)
    std::atomic<float> fartActivity { 0.0f };

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double currentSampleRate = 44100.0;
    float lowpassState[2] = { 0.0f, 0.0f };

    // Fart trigger detection
    FartVoice fartVoice;
    std::atomic<bool> manualTrigger { false };
    float follower = 0.0f;
    float followerRelease = 0.999f;
    bool armed = true;
    int cooldown = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmoothed, mixSmoothed, outputSmoothed, fartLevelSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FartexAudioProcessor)
};
