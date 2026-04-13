#pragma once

#include "KWeighting.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>

namespace stellarr::dsp
{

/**
 * LUFS meter computing momentary (400 ms) and short-term (3 s)
 * loudness per ITU-R BS.1770. Stereo input is summed equally.
 *
 * Thread model: process() runs on the audio thread, getMomentaryLufs()
 * and getShortTermLufs() are read from the message thread.
 */
class LoudnessMeter
{
public:
    static constexpr float kSilenceFloor = -60.0f;

    void prepare(double sampleRate, int numChannels)
    {
        sampleRateHz = sampleRate;
        kw.resize(static_cast<size_t>(numChannels));
        for (auto& f : kw) f.prepare(sampleRate);

        const auto momentarySamples = static_cast<int>(0.4 * sampleRate);
        const auto shortTermSamples = static_cast<int>(3.0 * sampleRate);

        momentaryBuffer.assign(static_cast<size_t>(momentarySamples), 0.0f);
        shortTermBuffer.assign(static_cast<size_t>(shortTermSamples), 0.0f);
        momentaryWriteIdx = 0;
        shortTermWriteIdx = 0;
        momentaryEnergy = 0.0f;
        shortTermEnergy = 0.0f;

        momentaryLufs.store(kSilenceFloor, std::memory_order_relaxed);
        shortTermLufs.store(kSilenceFloor, std::memory_order_relaxed);
    }

    void reset()
    {
        for (auto& f : kw) f.reset();
        std::fill(momentaryBuffer.begin(), momentaryBuffer.end(), 0.0f);
        std::fill(shortTermBuffer.begin(), shortTermBuffer.end(), 0.0f);
        momentaryWriteIdx = shortTermWriteIdx = 0;
        momentaryEnergy = shortTermEnergy = 0.0f;
        momentaryLufs.store(kSilenceFloor, std::memory_order_relaxed);
        shortTermLufs.store(kSilenceFloor, std::memory_order_relaxed);
    }

    void process(const juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin(buffer.getNumChannels(), static_cast<int>(kw.size()));
        const int numSamples = buffer.getNumSamples();
        if (numCh == 0 || numSamples == 0 || momentaryBuffer.empty()) return;

        for (int n = 0; n < numSamples; ++n)
        {
            // Sum K-weighted channels (equal weighting)
            float sumSquared = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float weighted = kw[static_cast<size_t>(ch)].processSample(buffer.getSample(ch, n));
                sumSquared += weighted * weighted;
            }

            // Push squared sample into both buffers (running mean energy)
            momentaryEnergy -= momentaryBuffer[momentaryWriteIdx];
            momentaryBuffer[momentaryWriteIdx] = sumSquared;
            momentaryEnergy += sumSquared;
            if (++momentaryWriteIdx >= momentaryBuffer.size()) momentaryWriteIdx = 0;

            shortTermEnergy -= shortTermBuffer[shortTermWriteIdx];
            shortTermBuffer[shortTermWriteIdx] = sumSquared;
            shortTermEnergy += sumSquared;
            if (++shortTermWriteIdx >= shortTermBuffer.size()) shortTermWriteIdx = 0;
        }

        momentaryLufs.store(energyToLufs(momentaryEnergy / static_cast<float>(momentaryBuffer.size())),
                            std::memory_order_relaxed);
        shortTermLufs.store(energyToLufs(shortTermEnergy / static_cast<float>(shortTermBuffer.size())),
                            std::memory_order_relaxed);
    }

    float getMomentaryLufs() const { return momentaryLufs.load(std::memory_order_relaxed); }
    float getShortTermLufs() const { return shortTermLufs.load(std::memory_order_relaxed); }

private:
    static float energyToLufs(float meanSquared)
    {
        if (meanSquared <= 1e-12f) return kSilenceFloor;
        const float lufs = -0.691f + 10.0f * std::log10(meanSquared);
        return juce::jmax(lufs, kSilenceFloor);
    }

    double sampleRateHz = 44100.0;
    std::vector<KWeighting> kw;
    std::vector<float> momentaryBuffer;
    std::vector<float> shortTermBuffer;
    size_t momentaryWriteIdx = 0;
    size_t shortTermWriteIdx = 0;
    float momentaryEnergy = 0.0f;
    float shortTermEnergy = 0.0f;
    std::atomic<float> momentaryLufs { kSilenceFloor };
    std::atomic<float> shortTermLufs { kSilenceFloor };
};

} // namespace stellarr::dsp
