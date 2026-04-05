#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/PluginBlock.h"
#include "../StellarrBridge.h"

class SessionTestAccess
{
public:
    static const auto& getBlockNodeMap(StellarrBridge& b) { return b.blockNodeMap; }
};

static bool testRestoreSessionMissingPlugin()
{
    printf("Test: restoreSession marks missing plugins... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    // Build a session JSON with a plugin block referencing a non-existent plugin
    auto session = juce::JSON::parse(R"({
        "version": 1,
        "grid": {"columns": 12, "rows": 6},
        "blocks": [
            {
                "id": "test-block-1",
                "type": "plugin",
                "name": "Plugin",
                "col": 3,
                "row": 2,
                "pluginId": "nonexistent-plugin-id-12345",
                "pluginName": "Deleted Amp Sim",
                "states": [{"pluginState":"","mix":0.5,"balance":0.0,"level":0.0,"bypassed":false,"bypassMode":"thru"}],
                "activeStateIndex": 0
            }
        ],
        "connections": [],
        "scenes": [],
        "activeSceneIndex": -1
    })");

    bridge.restoreSession(session);

    // Find the plugin block and check the missing flag
    auto& map = SessionTestAccess::getBlockNodeMap(bridge);
    if (map.empty())
    {
        printf("FAIL (no blocks restored)\n");
        proc.releaseResources();
        return false;
    }

    bool foundMissing = false;
    juce::String missingName;
    for (auto& [blockId, nodeId] : map)
    {
        if (auto* node = proc.getGraph().getNodeForId(nodeId))
        {
            if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                if (pb->isPluginMissing())
                {
                    foundMissing = true;
                    missingName = pb->getPluginName();
                }
            }
        }
    }

    if (!foundMissing)
    {
        printf("FAIL (pluginMissing not set)\n");
        proc.releaseResources();
        return false;
    }

    if (missingName != "Deleted Amp Sim")
    {
        printf("FAIL (expected name 'Deleted Amp Sim', got '%s')\n", missingName.toRawUTF8());
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

static bool testDeserialisation()
{
    printf("Test: deserialised block routes audio identically... ");

    auto original = std::make_unique<stellarr::GainBlock>();
    original->setGain(0.3f);
    auto json = original->toJson();

    auto restored = std::make_unique<stellarr::GainBlock>();
    restored->fromJson(json);

    if (std::abs(restored->getGain() - 0.3f) > 1e-6f)
    {
        fprintf(stderr, "  gain mismatch after deserialisation: %f\n",
                static_cast<double>(restored->getGain()));
        printf("FAIL\n");
        return false;
    }

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

static bool testSessionRoundTrip()
{
    printf("Test: full session serialise/restore round-trip... ");

    StellarrProcessor proc1;
    proc1.prepareToPlay(kSampleRate, kBlockSize);

    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.3f);

    proc1.disconnectBlocks(proc1.getAudioInputNodeId(), proc1.getAudioOutputNodeId());
    auto gainNode = proc1.addBlock(std::move(gain));
    proc1.connectBlocks(proc1.getAudioInputNodeId(), gainNode);
    proc1.connectBlocks(gainNode, proc1.getAudioOutputNodeId());

    juce::AudioBuffer<float> buf1(2, kTotalSamples);
    generateSine(buf1);
    processInBlocks(proc1, buf1);

    auto* node = proc1.getGraph().getNodeForId(gainNode);
    auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor());
    auto json = block->toJson();

    proc1.releaseResources();

    StellarrProcessor proc2;
    proc2.prepareToPlay(kSampleRate, kBlockSize);

    auto restoredGain = std::make_unique<stellarr::GainBlock>();
    restoredGain->fromJson(json);

    proc2.disconnectBlocks(proc2.getAudioInputNodeId(), proc2.getAudioOutputNodeId());
    auto restoredNode = proc2.addBlock(std::move(restoredGain));
    proc2.connectBlocks(proc2.getAudioInputNodeId(), restoredNode);
    proc2.connectBlocks(restoredNode, proc2.getAudioOutputNodeId());

    juce::AudioBuffer<float> buf2(2, kTotalSamples);
    generateSine(buf2);
    processInBlocks(proc2, buf2);

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

    juce::AudioBuffer<float> buf1(2, kTotalSamples);
    generateSine(buf1);
    juce::AudioBuffer<float> original(buf1);
    processInBlocks(proc, buf1);

    if (!compareBuffers(buf1, original, 1e-5f, 0.25f))
    {
        printf("FAIL (original)\n");
        return false;
    }

    int connectionCount = 0;
    for ([[maybe_unused]] auto& conn : proc.getGraph().getConnections())
        ++connectionCount;

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

int main()
{
    int failures = 0;

    if (!testRestoreSessionMissingPlugin()) ++failures;
    if (!testDeserialisation())            ++failures;
    if (!testSessionRoundTrip())           ++failures;
    if (!testSessionWithConnections())     ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
