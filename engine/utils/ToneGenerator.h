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

    // Load a WAV file for playback. Returns true on success.
    bool loadSample(const juce::File& file)
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr)
            return false;

        // Load into a temp buffer, then swap under lock
        juce::AudioBuffer<float> newBuffer(static_cast<int>(reader->numChannels),
                                            static_cast<int>(reader->lengthInSamples));
        reader->read(&newBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

        {
            juce::SpinLock::ScopedLockType lock(sampleLock);
            sampleBuffer = std::move(newBuffer);
            sampleRate = reader->sampleRate;
            samplePlaybackPos = 0;
            useSample = true;
            currentSampleFile = file.getFileNameWithoutExtension();
        }

        return true;
    }

    void clearSample()
    {
        juce::SpinLock::ScopedLockType lock(sampleLock);
        useSample = false;
        sampleBuffer.setSize(0, 0);
        currentSampleFile.clear();
    }

    bool isUsingSample() const { return useSample; }
    juce::String getCurrentSampleName() const { return currentSampleFile; }

    // Fill buffer with tone (synth melody or loaded sample)
    void fillBuffer(juce::AudioBuffer<float>& buffer)
    {
        juce::SpinLock::ScopedTryLockType lock(sampleLock);
        if (!lock.isLocked())
        {
            buffer.clear();
            return;
        }

        if (useSample && sampleBuffer.getNumSamples() > 0)
            fillFromSample(buffer);
        else
            fillFromSynth(buffer);
    }

private:
    void fillFromSample(juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        int sampleLen = sampleBuffer.getNumSamples();
        int sampleChannels = sampleBuffer.getNumChannels();

        // Resample ratio (sample rate conversion)
        double ratio = sampleRate / currentSampleRate;

        for (int i = 0; i < numSamples; ++i)
        {
            int srcPos = static_cast<int>(samplePlaybackPos * ratio) % sampleLen;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                int srcCh = std::min(ch, sampleChannels - 1);
                buffer.setSample(ch, i, sampleBuffer.getSample(srcCh, srcPos));
            }
            ++samplePlaybackPos;
            if (static_cast<int>(samplePlaybackPos * ratio) >= sampleLen)
                samplePlaybackPos = 0;
        }
    }

    void fillFromSynth(juce::AudioBuffer<float>& buffer)
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
