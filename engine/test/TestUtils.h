#pragma once
#include "StellarrProcessor.h"
#include <cmath>
#include <cstdio>

static constexpr double kSampleRate = 44100.0;
static constexpr int kBlockSize = 512;
static constexpr int kTotalSamples = static_cast<int>(kSampleRate);
static constexpr float kFrequency = 440.0f;
static constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

static inline void generateSine(juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i,
                std::sin(kTwoPi * kFrequency * static_cast<float>(i)
                         / static_cast<float>(kSampleRate)));
}

static inline void processInBlocks(StellarrProcessor& proc, juce::AudioBuffer<float>& buffer)
{
    for (int offset = 0; offset < buffer.getNumSamples(); offset += kBlockSize)
    {
        int count = std::min(kBlockSize, buffer.getNumSamples() - offset);
        float* channels[2] = {
            buffer.getWritePointer(0) + offset,
            buffer.getWritePointer(1) + offset,
        };
        juce::AudioBuffer<float> block(channels, 2, count);
        juce::MidiBuffer midi;
        proc.processBlock(block, midi);
    }
}

static inline bool compareBuffers(const juce::AudioBuffer<float>& a,
                                  const juce::AudioBuffer<float>& b,
                                  float tolerance, float expectedScale = 1.0f)
{
    for (int ch = 0; ch < a.getNumChannels(); ++ch)
    {
        for (int i = 0; i < a.getNumSamples(); ++i)
        {
            float expected = b.getSample(ch, i) * expectedScale;
            float diff = std::abs(a.getSample(ch, i) - expected);

            if (diff > tolerance)
            {
                fprintf(stderr,
                    "  mismatch at ch=%d sample=%d (got %f, expected %f)\n",
                    ch, i,
                    static_cast<double>(a.getSample(ch, i)),
                    static_cast<double>(expected));
                return false;
            }
        }
    }
    return true;
}
