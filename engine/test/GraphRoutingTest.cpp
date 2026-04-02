#include "TestUtils.h"
#include "blocks/GainBlock.h"

static bool testSeriesRouting()
{
    printf("Test: series routing (two gain blocks)... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain1 = std::make_unique<stellarr::GainBlock>();
    auto gain2 = std::make_unique<stellarr::GainBlock>();
    gain1->setGain(0.5f);
    gain2->setGain(0.5f);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

    auto node1 = proc.addBlock(std::move(gain1));
    auto node2 = proc.addBlock(std::move(gain2));

    proc.connectBlocks(proc.getAudioInputNodeId(), node1);
    proc.connectBlocks(node1, node2);
    proc.connectBlocks(node2, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> original(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, original, 1e-5f, 0.25f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testParallelRouting()
{
    printf("Test: parallel routing (two gain blocks merged)... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gainA = std::make_unique<stellarr::GainBlock>();
    auto gainB = std::make_unique<stellarr::GainBlock>();
    gainA->setGain(0.5f);
    gainB->setGain(0.5f);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

    auto nodeA = proc.addBlock(std::move(gainA));
    auto nodeB = proc.addBlock(std::move(gainB));

    proc.connectBlocks(proc.getAudioInputNodeId(), nodeA);
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeB);
    proc.connectBlocks(nodeA, proc.getAudioOutputNodeId());
    proc.connectBlocks(nodeB, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> original(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, original, 1e-5f, 1.0f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testDisconnectedBlock()
{
    printf("Test: disconnected block receives no audio... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto orphan = std::make_unique<stellarr::GainBlock>();
    orphan->setGain(2.0f);
    proc.addBlock(std::move(orphan));

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

static bool testReconnection()
{
    printf("Test: disconnect and reconnect mid-stream... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto gainNode = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNode);
    proc.connectBlocks(gainNode, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buf1(2, kBlockSize);
    generateSine(buf1);
    juce::MidiBuffer midi;
    proc.processBlock(buf1, midi);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), gainNode);
    proc.disconnectBlocks(gainNode, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buf2(2, kBlockSize);
    generateSine(buf2);
    proc.processBlock(buf2, midi);

    proc.connectBlocks(proc.getAudioInputNodeId(), gainNode);
    proc.connectBlocks(gainNode, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buf3(2, kBlockSize);
    generateSine(buf3);
    juce::AudioBuffer<float> original(buf3);
    proc.processBlock(buf3, midi);

    if (!compareBuffers(buf3, original, 1e-5f, 0.5f))
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

    if (!testSeriesRouting())     ++failures;
    if (!testParallelRouting())   ++failures;
    if (!testDisconnectedBlock()) ++failures;
    if (!testReconnection())      ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
