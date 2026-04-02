#include "TestUtils.h"
#include "blocks/GainBlock.h"

static bool testMultipleRemoves()
{
    printf("Test: remove middle block, reconnect remaining... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto g1 = std::make_unique<stellarr::GainBlock>();
    auto g2 = std::make_unique<stellarr::GainBlock>();
    auto g3 = std::make_unique<stellarr::GainBlock>();
    g1->setGain(0.5f);
    g2->setGain(0.5f);
    g3->setGain(0.5f);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

    auto n1 = proc.addBlock(std::move(g1));
    auto n2 = proc.addBlock(std::move(g2));
    auto n3 = proc.addBlock(std::move(g3));

    proc.connectBlocks(proc.getAudioInputNodeId(), n1);
    proc.connectBlocks(n1, n2);
    proc.connectBlocks(n2, n3);
    proc.connectBlocks(n3, proc.getAudioOutputNodeId());

    proc.removeBlock(n2);
    proc.connectBlocks(n1, n3);

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

static bool testRemoveAllBlocks()
{
    printf("Test: remove all user blocks produces silence... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    proc.removeBlock(nodeId);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (std::abs(buffer.getSample(ch, i)) > 1e-6f)
            {
                fprintf(stderr, "  expected silence at ch=%d sample=%d, got %f\n",
                        ch, i, static_cast<double>(buffer.getSample(ch, i)));
                printf("FAIL\n");
                return false;
            }
        }
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testDuplicateConnections()
{
    printf("Test: duplicate connection does not double audio... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(gain));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

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

static bool testZeroLengthBuffer()
{
    printf("Test: zero-length buffer does not crash... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, 0);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testRapidAddRemove()
{
    printf("Test: rapid add/remove 50 blocks... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    std::vector<juce::AudioProcessorGraph::NodeID> nodes;

    for (int i = 0; i < 50; ++i)
    {
        auto gain = std::make_unique<stellarr::GainBlock>();
        auto nodeId = proc.addBlock(std::move(gain));
        nodes.push_back(nodeId);
    }

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    for (auto& nodeId : nodes)
        proc.removeBlock(nodeId);

    buffer.clear();
    generateSine(buffer);
    proc.processBlock(buffer, midi);

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testMultipleRemoves())      ++failures;
    if (!testRemoveAllBlocks())      ++failures;
    if (!testDuplicateConnections()) ++failures;
    if (!testZeroLengthBuffer())     ++failures;
    if (!testRapidAddRemove())       ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
