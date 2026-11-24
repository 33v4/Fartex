#include "PluginProcessor.h"
#include "PluginEditor.h"

FartexAudioProcessor::FartexAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout FartexAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto oneDecimal = [] (float v, int) { return juce::String (v, 1); };
    auto wholeNumber = [] (float v, int) { return juce::String (juce::roundToInt (v)); };

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> (0.0f, 40.0f, 0.01f), 12.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (oneDecimal)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (200.0f, 20000.0f, 1.0f, 0.3f), 8000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (wholeNumber)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (wholeNumber)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (oneDecimal)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fartlevel", 1 }, "Fart Level",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction (wholeNumber)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "trigger", 1 }, "Trigger",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -24.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (oneDecimal)));

    return layout;
}

void FartexAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;

    lowpassState[0] = lowpassState[1] = 0.0f;

    fartVoice.prepare (sampleRate);
    follower = 0.0f;
    armed = true;
    cooldown = 0;
    followerRelease = std::exp (-1.0f / (0.08f * (float) sampleRate));

    driveSmoothed.reset      (sampleRate, 0.02);
    mixSmoothed.reset        (sampleRate, 0.02);
    outputSmoothed.reset     (sampleRate, 0.02);
    fartLevelSmoothed.reset  (sampleRate, 0.02);

    driveSmoothed.setCurrentAndTargetValue     (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("drive")->load()));
    mixSmoothed.setCurrentAndTargetValue       (apvts.getRawParameterValue ("mix")->load() / 100.0f);
    outputSmoothed.setCurrentAndTargetValue    (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    fartLevelSmoothed.setCurrentAndTargetValue (apvts.getRawParameterValue ("fartlevel")->load() / 100.0f * 1.5f);
}

bool FartexAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void FartexAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);

    // Read the knobs once per block
    const float driveDb    = apvts.getRawParameterValue ("drive")->load();
    const float aggression = juce::jlimit (0.0f, 1.0f, driveDb / 40.0f);   // more drive = angrier farts
    const float threshold  = juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("trigger")->load());

    driveSmoothed.setTargetValue     (juce::Decibels::decibelsToGain (driveDb));
    mixSmoothed.setTargetValue       (apvts.getRawParameterValue ("mix")->load() / 100.0f);
    outputSmoothed.setTargetValue    (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    fartLevelSmoothed.setTargetValue (apvts.getRawParameterValue ("fartlevel")->load() / 100.0f * 1.5f);

    // Tone = simple one-pole low-pass filter
    const float cutoff = apvts.getRawParameterValue ("tone")->load();
    const float coeff  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) currentSampleRate);

    // Did somebody click the butt?
    if (manualTrigger.exchange (false))
        fartVoice.trigger (aggression);

    float* channelData[2] = { nullptr, nullptr };
    for (int ch = 0; ch < numChannels; ++ch)
        channelData[ch] = buffer.getWritePointer (ch);

    float blockActivity = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- Listen to the input: loud hits make the butt go off ---
        float inLevel = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            inLevel = juce::jmax (inLevel, std::abs (channelData[ch][i]));

        follower = (inLevel > follower) ? inLevel : follower * followerRelease;

        if (cooldown > 0)
            --cooldown;

        if (follower < threshold * 0.5f)
        {
            armed = true;
        }
        else if (armed && cooldown == 0 && follower > threshold)
        {
            fartVoice.trigger (aggression);
            armed = false;
            cooldown = (int) (0.15 * currentSampleRate);
        }

        const float drive    = driveSmoothed.getNextValue();
        const float mix      = mixSmoothed.getNextValue();
        const float outGain  = outputSmoothed.getNextValue();
        const float fartGain = fartLevelSmoothed.getNextValue();

        const float fartSample = fartVoice.process() * fartGain;
        blockActivity = juce::jmax (blockActivity, std::abs (fartSample));

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = channelData[ch][i];

            // Distortion: soft clipping
            float wet = std::tanh (dry * drive);

            // Tone filter
            lowpassState[ch] += coeff * (wet - lowpassState[ch]);
            wet = lowpassState[ch];

            channelData[ch][i] = ((dry * (1.0f - mix) + wet * mix) + fartSample) * outGain;
        }
    }

    fartActivity.store (blockActivity);
}

juce::AudioProcessorEditor* FartexAudioProcessor::createEditor()
{
    return new FartexAudioProcessorEditor (*this);
}

void FartexAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FartexAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FartexAudioProcessor();
}
