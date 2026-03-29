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

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
