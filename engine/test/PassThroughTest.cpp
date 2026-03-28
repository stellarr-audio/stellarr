#include "StellarrProcessor.h"
#include <cmath>
#include <cstdio>

int main()
{
    StellarrProcessor processor;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int totalSamples = static_cast<int>(sampleRate);
    constexpr float frequency = 440.0f;
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    processor.prepareToPlay(sampleRate, blockSize);

    juce::AudioBuffer<float> buffer(2, totalSamples);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < totalSamples; ++i)
            buffer.setSample(ch, i,
                std::sin(twoPi * frequency * static_cast<float>(i)
                         / static_cast<float>(sampleRate)));

    juce::AudioBuffer<float> expected(buffer);

    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        int count = std::min(blockSize, totalSamples - offset);

        float* channels[2] = {
            buffer.getWritePointer(0) + offset,
            buffer.getWritePointer(1) + offset,
        };

        juce::AudioBuffer<float> block(channels, 2, count);
        juce::MidiBuffer midi;
        processor.processBlock(block, midi);
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < totalSamples; ++i)
        {
            float diff = std::abs(buffer.getSample(ch, i)
                                  - expected.getSample(ch, i));

            if (diff > 1e-6f)
            {
                fprintf(stderr,
                    "FAIL: sample mismatch at ch=%d sample=%d "
                    "(got %f, expected %f)\n",
                    ch, i,
                    static_cast<double>(buffer.getSample(ch, i)),
                    static_cast<double>(expected.getSample(ch, i)));
                return 1;
            }
        }
    }

    printf("PASS: audio pass-through verified "
           "(%d samples, stereo, 440 Hz sine)\n", totalSamples);

    processor.releaseResources();
    return 0;
}
