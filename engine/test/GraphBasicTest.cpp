#include "TestUtils.h"
#include "blocks/GainBlock.h"

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

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

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

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto gainNodeId = proc.addBlock(std::move(gainBlock));
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNodeId);
    proc.connectBlocks(gainNodeId, proc.getAudioOutputNodeId());

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

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto gainBlock = std::make_unique<stellarr::GainBlock>();
    gainBlock->setGain(1.0f);
    auto gainNodeId = proc.addBlock(std::move(gainBlock));
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNodeId);
    proc.connectBlocks(gainNodeId, proc.getAudioOutputNodeId());

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
