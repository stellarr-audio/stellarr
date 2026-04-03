#pragma once
#include <cmath>
#include <juce_core/juce_core.h>

namespace stellarr
{

class ToneGenerator
{
public:
    struct Note
    {
        double frequency;
        double beats;
    };

    void prepareToPlay(double sampleRate, double bpm = 120.0)
    {
        currentSampleRate = sampleRate;
        samplesPerBeat = sampleRate * 60.0 / bpm;
    }

    void reset()
    {
        phase = 0.0;
        melodyIndex = 0;
        sampleInNote = 0.0;
    }

    // Fill buffer with tone. Overwrites all channels with the same mono signal.
    void fillBuffer(juce::AudioBuffer<float>& buffer)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            auto noteLength = melody[melodyIndex].beats * samplesPerBeat;
            if (sampleInNote >= noteLength)
            {
                sampleInNote = 0.0;
                melodyIndex = (melodyIndex + 1) % melodySize;
                phase = 0.0;
            }

            auto freq = melody[melodyIndex].frequency;
            float sample = 0.0f;

            if (freq > 0.0)
            {
                auto noteProgress = sampleInNote / noteLength;
                auto envelope = static_cast<float>(
                    noteProgress < 0.02 ? noteProgress / 0.02
                    : 1.0 - noteProgress * 0.4);
                if (envelope < 0.0f) envelope = 0.0f;

                sample = envelope * amplitude * static_cast<float>(
                    std::sin(phase * juce::MathConstants<double>::twoPi));

                phase += freq / currentSampleRate;
                if (phase >= 1.0) phase -= 1.0;
            }

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, sample);

            sampleInNote += 1.0;
        }
    }

private:
    // A short pentatonic riff in E minor — loops naturally
    static constexpr int melodySize = 12;
    static constexpr Note melody[melodySize] = {
        {329.63, 0.5},   // E4
        {392.00, 0.5},   // G4
        {440.00, 0.5},   // A4
        {493.88, 1.0},   // B4
        {440.00, 0.5},   // A4
        {392.00, 0.5},   // G4
        {329.63, 1.0},   // E4
        {0.0,    0.5},   // rest
        {293.66, 0.5},   // D4
        {329.63, 0.5},   // E4
        {392.00, 1.0},   // G4
        {0.0,    1.0},   // rest
    };

    static constexpr float amplitude = 0.45f;

    double phase = 0.0;
    double currentSampleRate = 44100.0;
    double samplesPerBeat = 44100.0 * 60.0 / 120.0;
    int melodyIndex = 0;
    double sampleInNote = 0.0;
};

} // namespace stellarr
