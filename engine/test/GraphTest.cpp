#include "StellarrProcessor.h"
#include "blocks/GainBlock.h"
#include <cmath>
#include <cstdio>

static constexpr double kSampleRate = 44100.0;
static constexpr int kBlockSize = 512;
static constexpr int kTotalSamples = static_cast<int>(kSampleRate);
static constexpr float kFrequency = 440.0f;
static constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

static void generateSine(juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i,
                std::sin(kTwoPi * kFrequency * static_cast<float>(i)
                         / static_cast<float>(kSampleRate)));
}

static void processInBlocks(StellarrProcessor& proc, juce::AudioBuffer<float>& buffer)
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

static bool compareBuffers(const juce::AudioBuffer<float>& a,
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

// ---------------------------------------------------------------------------

static bool testPassThrough()
{
    printf("Test: default graph pass-through... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testGainBlock()
{
    printf("Test: gain block at 0.5... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gainBlock = std::make_unique<stellarr::GainBlock>();
    gainBlock->setGain(0.5f);

    // Disconnect default input → output
    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

    // Insert gain: input → gain → output
    auto gainNodeId = proc.addBlock(std::move(gainBlock));
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNodeId);
    proc.connectBlocks(gainNodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> original(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, original, 1e-5f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testRemoveBlock()
{
    printf("Test: remove gain block restores pass-through... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gainBlock = std::make_unique<stellarr::GainBlock>();
    gainBlock->setGain(0.5f);

    // Insert gain block
    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto gainNodeId = proc.addBlock(std::move(gainBlock));
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNodeId);
    proc.connectBlocks(gainNodeId, proc.getAudioOutputNodeId());

    // Remove gain block and reconnect input → output
    proc.removeBlock(gainNodeId);
    proc.connectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, expected, 1e-6f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testAddDuringProcessing()
{
    printf("Test: add block between process calls... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    // Process one block with default graph
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Add a gain block between process calls
    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto gainBlock = std::make_unique<stellarr::GainBlock>();
    gainBlock->setGain(1.0f);
    auto gainNodeId = proc.addBlock(std::move(gainBlock));
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNodeId);
    proc.connectBlocks(gainNodeId, proc.getAudioOutputNodeId());

    // Process another block — must not crash
    juce::AudioBuffer<float> buffer2(2, kBlockSize);
    generateSine(buffer2);
    juce::AudioBuffer<float> expected(buffer2);
    proc.processBlock(buffer2, midi);

    if (!compareBuffers(buffer2, expected, 1e-5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// ---------------------------------------------------------------------------

int main()
{
    int failures = 0;

    if (!testPassThrough())        ++failures;
    if (!testGainBlock())          ++failures;
    if (!testRemoveBlock())        ++failures;
    if (!testAddDuringProcessing()) ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
