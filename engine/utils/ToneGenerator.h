#pragma once
#include <cmath>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
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

    void prepareToPlay(double hostSampleRate, double bpm = 120.0)
    {
        currentSampleRate = hostSampleRate;
        samplesPerBeat = hostSampleRate * 60.0 / bpm;
    }

    void reset()
    {
        phase = 0.0;
        melodyIndex = 0;
        sampleInNote = 0.0;
        samplePlaybackPos = 0;
    }

    bool loadSample(const juce::File& file);

    void clearSample()
    {
        juce::SpinLock::ScopedLockType lock(sampleLock);
        useSample = false;
        sampleBuffer.setSize(0, 0);
        currentSampleFile.clear();
    }

    bool isUsingSample() const { return useSample; }
    juce::String getCurrentSampleName() const { return currentSampleFile; }

    void fillBuffer(juce::AudioBuffer<float>& buffer);

private:
    void fillFromSample(juce::AudioBuffer<float>& buffer);
    void fillFromSynth(juce::AudioBuffer<float>& buffer);

    // Synth melody data
    static constexpr int melodySize = 12;
    static constexpr Note melody[melodySize] = {
        {329.63, 0.5}, {392.00, 0.5}, {440.00, 0.5}, {493.88, 1.0},
        {440.00, 0.5}, {392.00, 0.5}, {329.63, 1.0}, {0.0, 0.5},
        {293.66, 0.5}, {329.63, 0.5}, {392.00, 1.0}, {0.0, 1.0},
    };
    static constexpr float amplitude = 0.45f;

    // Synth state
    double phase = 0.0;
    double currentSampleRate = 44100.0;
    double samplesPerBeat = 44100.0 * 60.0 / 120.0;
    int melodyIndex = 0;
    double sampleInNote = 0.0;

    // Sample state
    juce::SpinLock sampleLock;
    bool useSample = false;
    juce::AudioBuffer<float> sampleBuffer;
    double sampleRate = 44100.0;
    int64_t samplePlaybackPos = 0;
    juce::String currentSampleFile;
};

} // namespace stellarr
