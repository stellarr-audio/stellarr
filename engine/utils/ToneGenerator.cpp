#include "ToneGenerator.h"

namespace stellarr
{

bool ToneGenerator::loadSample(const juce::File& file)
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

void ToneGenerator::fillBuffer(juce::AudioBuffer<float>& buffer)
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

void ToneGenerator::fillFromSample(juce::AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    int sampleLen = sampleBuffer.getNumSamples();
    int sampleChannels = sampleBuffer.getNumChannels();

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

void ToneGenerator::fillFromSynth(juce::AudioBuffer<float>& buffer)
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

} // namespace stellarr
