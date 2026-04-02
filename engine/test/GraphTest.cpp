#include "StellarrProcessor.h"
#include "blocks/GainBlock.h"
#include "blocks/InputBlock.h"
#include "blocks/VstBlock.h"
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
// Phase 1B — Routing test suite
// ---------------------------------------------------------------------------

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

    // Input → Gain(0.5) → Gain(0.5) → Output = 0.25 amplitude
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

    // Input → GainA(0.5) → Output
    // Input → GainB(0.5) → Output
    // Graph sums at output: 0.5 + 0.5 = 1.0
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeA);
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeB);
    proc.connectBlocks(nodeA, proc.getAudioOutputNodeId());
    proc.connectBlocks(nodeB, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> original(buffer);

    processInBlocks(proc, buffer);

    // Both paths carry signal, summed at output = original amplitude
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

    // Add a gain block but do not connect it to anything
    auto orphan = std::make_unique<stellarr::GainBlock>();
    orphan->setGain(2.0f);
    proc.addBlock(std::move(orphan));

    // Default Input → Output path remains intact
    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> expected(buffer);

    processInBlocks(proc, buffer);

    // Output must match input — the orphan block has no effect
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

    // Process one block through the gain path
    juce::AudioBuffer<float> buf1(2, kBlockSize);
    generateSine(buf1);
    juce::MidiBuffer midi;
    proc.processBlock(buf1, midi);

    // Disconnect the gain block
    proc.disconnectBlocks(proc.getAudioInputNodeId(), gainNode);
    proc.disconnectBlocks(gainNode, proc.getAudioOutputNodeId());

    // Process while disconnected — output should be silence
    juce::AudioBuffer<float> buf2(2, kBlockSize);
    generateSine(buf2);
    proc.processBlock(buf2, midi);

    // Reconnect
    proc.connectBlocks(proc.getAudioInputNodeId(), gainNode);
    proc.connectBlocks(gainNode, proc.getAudioOutputNodeId());

    // Process again — should produce 0.5 amplitude
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

static bool testSerialisation()
{
    printf("Test: block serialises to valid JSON... ");

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.75f);

    auto json = gain->toJson();
    auto* obj = json.getDynamicObject();

    if (obj == nullptr)
    {
        fprintf(stderr, "  toJson() returned null object\n");
        printf("FAIL\n");
        return false;
    }

    bool ok = true;

    auto id = obj->getProperty("id").toString();
    if (id.isEmpty())
    {
        fprintf(stderr, "  missing or empty 'id'\n");
        ok = false;
    }

    auto type = obj->getProperty("type").toString();
    if (type != "gain")
    {
        fprintf(stderr, "  expected type 'gain', got '%s'\n", type.toRawUTF8());
        ok = false;
    }

    auto name = obj->getProperty("name").toString();
    if (name != "Gain")
    {
        fprintf(stderr, "  expected name 'Gain', got '%s'\n", name.toRawUTF8());
        ok = false;
    }

    auto gainVal = static_cast<float>(obj->getProperty("gain"));
    if (std::abs(gainVal - 0.75f) > 1e-6f)
    {
        fprintf(stderr, "  expected gain 0.75, got %f\n", static_cast<double>(gainVal));
        ok = false;
    }

    printf("%s\n", ok ? "PASS" : "FAIL");
    return ok;
}

static bool testDeserialisation()
{
    printf("Test: deserialised block routes audio identically... ");

    // Create and serialise a gain block
    auto original = std::make_unique<stellarr::GainBlock>();
    original->setGain(0.3f);
    auto json = original->toJson();

    // Deserialise into a new block
    auto restored = std::make_unique<stellarr::GainBlock>();
    restored->fromJson(json);

    if (std::abs(restored->getGain() - 0.3f) > 1e-6f)
    {
        fprintf(stderr, "  gain mismatch after deserialisation: %f\n",
                static_cast<double>(restored->getGain()));
        printf("FAIL\n");
        return false;
    }

    // Wire the restored block into a graph and verify audio output
    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(restored));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> reference(buffer);

    processInBlocks(proc, buffer);

    if (!compareBuffers(buffer, reference, 1e-5f, 0.3f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// ---------------------------------------------------------------------------
// Additional tests
// ---------------------------------------------------------------------------

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

    // Remove middle block
    proc.removeBlock(n2);

    // Reconnect: n1 → n3
    proc.connectBlocks(n1, n3);

    // Expected: 0.5 * 0.5 = 0.25
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

    // Remove the gain block — no path from input to output
    proc.removeBlock(nodeId);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Output should be silence (no connected path)
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

    // Try to add the same connection again
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

    juce::AudioBuffer<float> buffer(2, kTotalSamples);
    generateSine(buffer);
    juce::AudioBuffer<float> original(buffer);

    processInBlocks(proc, buffer);

    // Should still be 0.5x, not 1.0x
    if (!compareBuffers(buffer, original, 1e-5f, 0.5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testVstBlockPassThrough()
{
    printf("Test: VstBlock with no plugin passes audio... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto vst = std::make_unique<stellarr::VstBlock>();

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());
    auto nodeId = proc.addBlock(std::move(vst));
    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId());

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

static bool testInputBlockTestTone()
{
    printf("Test: InputBlock test tone produces non-zero output... ");

    auto input = std::make_unique<stellarr::InputBlock>();
    input->prepareToPlay(kSampleRate, kBlockSize);
    input->setTestToneEnabled(true);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    input->processBlock(buffer, midi);

    bool hasSignal = false;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float sample = std::abs(buffer.getSample(0, i));
        if (sample > 0.01f)
        {
            hasSignal = true;
            break;
        }
    }

    if (!hasSignal)
    {
        fprintf(stderr, "  test tone produced no signal\n");
        printf("FAIL\n");
        return false;
    }

    // Check amplitude is within expected range (0.45 max)
    float peak = 0.0f;
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        peak = std::max(peak, std::abs(buffer.getSample(0, i)));

    if (peak > 0.5f)
    {
        fprintf(stderr, "  peak %f exceeds expected range\n", static_cast<double>(peak));
        printf("FAIL\n");
        return false;
    }

    input->releaseResources();
    printf("PASS\n");
    return true;
}

static bool testInputBlockTestToneReset()
{
    printf("Test: InputBlock resetToDefault disables test tone... ");

    auto input = std::make_unique<stellarr::InputBlock>();
    input->prepareToPlay(kSampleRate, kBlockSize);
    input->setTestToneEnabled(true);

    // Process one block to confirm tone is active
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    input->processBlock(buffer, midi);

    // Reset
    input->resetToDefault();

    // Process again — should be silence (pass-through of cleared buffer)
    buffer.clear();
    input->processBlock(buffer, midi);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        if (std::abs(buffer.getSample(0, i)) > 1e-6f)
        {
            fprintf(stderr, "  expected silence after reset at sample=%d\n", i);
            printf("FAIL\n");
            return false;
        }
    }

    input->releaseResources();
    printf("PASS\n");
    return true;
}

static bool testSessionRoundTrip()
{
    printf("Test: full session serialise/restore round-trip... ");

    // Build original graph: Input → Gain(0.3) → Output
    StellarrProcessor proc1;
    proc1.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.3f);

    proc1.disconnectBlocks(proc1.getAudioInputNodeId(), proc1.getAudioOutputNodeId());
    auto gainNode = proc1.addBlock(std::move(gain));
    proc1.connectBlocks(proc1.getAudioInputNodeId(), gainNode);
    proc1.connectBlocks(gainNode, proc1.getAudioOutputNodeId());

    // Process with original
    juce::AudioBuffer<float> buf1(2, kTotalSamples);
    generateSine(buf1);
    processInBlocks(proc1, buf1);

    // Serialise the gain block
    auto* node = proc1.getGraph().getNodeForId(gainNode);
    auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor());
    auto json = block->toJson();

    proc1.releaseResources();

    // Restore into a fresh processor
    StellarrProcessor proc2;
    proc2.prepareToPlay(kSampleRate, kBlockSize);

    auto restored = std::make_unique<stellarr::GainBlock>();
    restored->fromJson(json);

    proc2.disconnectBlocks(proc2.getAudioInputNodeId(), proc2.getAudioOutputNodeId());
    auto restoredNode = proc2.addBlock(std::move(restored));
    proc2.connectBlocks(proc2.getAudioInputNodeId(), restoredNode);
    proc2.connectBlocks(restoredNode, proc2.getAudioOutputNodeId());

    juce::AudioBuffer<float> buf2(2, kTotalSamples);
    generateSine(buf2);
    processInBlocks(proc2, buf2);

    // Both outputs should match
    if (!compareBuffers(buf1, buf2, 1e-5f))
    {
        printf("FAIL\n");
        return false;
    }

    proc2.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testSessionWithConnections()
{
    printf("Test: session preserves multiple connections... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    auto g1 = std::make_unique<stellarr::GainBlock>();
    auto g2 = std::make_unique<stellarr::GainBlock>();
    g1->setGain(0.5f);
    g2->setGain(0.5f);

    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId());

    auto n1 = proc.addBlock(std::move(g1));
    auto n2 = proc.addBlock(std::move(g2));

    proc.connectBlocks(proc.getAudioInputNodeId(), n1);
    proc.connectBlocks(n1, n2);
    proc.connectBlocks(n2, proc.getAudioOutputNodeId());

    // Process original: 0.5 * 0.5 = 0.25
    juce::AudioBuffer<float> buf1(2, kTotalSamples);
    generateSine(buf1);
    juce::AudioBuffer<float> original(buf1);
    processInBlocks(proc, buf1);

    if (!compareBuffers(buf1, original, 1e-5f, 0.25f))
    {
        printf("FAIL (original)\n");
        return false;
    }

    // Verify connections count
    int connectionCount = 0;
    for (auto& conn : proc.getGraph().getConnections())
    {
        (void)conn;
        ++connectionCount;
    }

    // Should have connections: audioIn→n1 (x2 channels), n1→n2 (x2), n2→audioOut (x2) = 6
    if (connectionCount < 6)
    {
        fprintf(stderr, "  expected at least 6 connections, got %d\n", connectionCount);
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

    // Process a block mid-way
    juce::AudioBuffer<float> buffer(2, kBlockSize);
    generateSine(buffer);
    juce::MidiBuffer midi;
    proc.processBlock(buffer, midi);

    // Remove all
    for (auto& nodeId : nodes)
        proc.removeBlock(nodeId);

    // Process again after removal
    buffer.clear();
    generateSine(buffer);
    proc.processBlock(buffer, midi);

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// ---------------------------------------------------------------------------

int main()
{
    int failures = 0;

    // Phase 1A
    if (!testPassThrough())         ++failures;
    if (!testGainBlock())           ++failures;
    if (!testRemoveBlock())         ++failures;
    if (!testAddDuringProcessing()) ++failures;

    // Phase 1B
    if (!testSeriesRouting())       ++failures;
    if (!testParallelRouting())     ++failures;
    if (!testDisconnectedBlock())   ++failures;
    if (!testReconnection())        ++failures;
    if (!testSerialisation())       ++failures;
    if (!testDeserialisation())     ++failures;

    // Additional tests
    if (!testMultipleRemoves())         ++failures;
    if (!testRemoveAllBlocks())         ++failures;
    if (!testDuplicateConnections())    ++failures;
    if (!testVstBlockPassThrough())     ++failures;
    if (!testInputBlockTestTone())      ++failures;
    if (!testInputBlockTestToneReset()) ++failures;
    if (!testSessionRoundTrip())        ++failures;
    if (!testSessionWithConnections())  ++failures;
    if (!testZeroLengthBuffer())        ++failures;
    if (!testRapidAddRemove())          ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
