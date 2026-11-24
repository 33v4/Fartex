#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// A tiny procedural fart synthesizer.
// aggression: 0 = polite little pfft, 1 = absolute disaster.
class FartVoice
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;
        active = false;
    }

    bool isActive() const { return active; }

    void trigger (float aggression)
    {
        const float a = juce::jlimit (0.0f, 1.0f, aggression);

        active        = true;
        age           = 0;
        lengthSamples = juce::jmax (1, (int) (sampleRate * (0.30 + 0.55 * a)));

        baseFreq  = 40.0f + 90.0f * a;     // flap rate (raspier when aggressive)
        jitter    = 0.06f + 0.40f * a;     // pitch instability
        sputter   = 0.10f + 0.50f * a;     // uneven pulse strength
        noiseAmt  = 0.06f + 0.55f * a;     // wetness
        distGain  = 1.5f + 10.0f * a;      // crunch

        const float cutoff = 500.0f + 4000.0f * a;
        lpCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cutoff / (float) sampleRate);
        dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 20.0f / (float) sampleRate;

        phase = 0.0f;
        jitterFactor = 1.0f;
        pulseAmp = 1.0f;
        lp1 = lp2 = dcX = dcY = 0.0f;
    }

    float process()
    {
        if (! active)
            return 0.0f;

        const float t = (float) age / (float) lengthSamples;

        if (t >= 1.0f)
        {
            active = false;
            return 0.0f;
        }

        // Pitch drops over the length of the fart
        const float freq = baseFreq * jitterFactor * (1.35f - 0.7f * t);
        phase += freq / (float) sampleRate;

        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            jitterFactor = 1.0f + jitter * (random.nextFloat() * 2.0f - 1.0f);
            pulseAmp     = 1.0f - sputter * random.nextFloat();
        }

        // Sharp decaying spike every cycle = the "flap"
        const float spike = (std::exp (-9.0f * phase) - 0.111f) * pulseAmp;
        const float noise = random.nextFloat() * 2.0f - 1.0f;

        float x = spike * 2.5f + noise * noiseAmt * (0.3f + std::exp (-6.0f * phase));

        // Crunch
        x = std::tanh (x * distGain);

        // Two low-pass filters for a rounder, wetter tone
        lp1 += lpCoeff * (x - lp1);
        lp2 += lpCoeff * (lp1 - lp2);

        // Remove DC offset
        const float dcOut = lp2 - dcX + dcCoeff * dcY;
        dcX = lp2;
        dcY = dcOut;

        // Envelope: quick attack, curved decay
        const float attack = juce::jmin (1.0f, (float) age / (0.004f * (float) sampleRate));
        const float decay  = std::pow (1.0f - t, 1.3f);

        ++age;

        return dcOut * attack * decay;
    }

private:
    double sampleRate = 44100.0;
    bool active = false;
    int age = 0, lengthSamples = 1;

    float baseFreq = 60.0f, jitter = 0.1f, sputter = 0.1f, noiseAmt = 0.1f, distGain = 2.0f;
    float lpCoeff = 0.1f, dcCoeff = 0.995f;
    float phase = 0.0f, jitterFactor = 1.0f, pulseAmp = 1.0f;
    float lp1 = 0.0f, lp2 = 0.0f, dcX = 0.0f, dcY = 0.0f;

    juce::Random random;
};
