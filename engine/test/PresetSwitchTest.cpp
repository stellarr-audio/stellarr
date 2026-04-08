#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/PluginBlock.h"
#include "../StellarrBridge.h"

class PresetSwitchTestAccess
{
public:
    static const auto& getBlockNodeMap(StellarrBridge& b) { return b.blockNodeMap; }
};

// -- Batched graph rebuild ----------------------------------------------------

static bool testBatchedRebuildRoutesAudio()
{
    printf("Test: batched UpdateKind::none + rebuild routes audio... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    using UK = StellarrProcessor::UpdateKind;

    // Remove default input->output connection without rebuilding
    proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId(), UK::none);

    // Add a gain block without triggering intermediate rebuilds
    auto gain = std::make_unique<stellarr::GainBlock>();
    gain->setGain(0.5f);
    auto nodeId = proc.addBlock(std::move(gain), UK::none);

    proc.connectBlocks(proc.getAudioInputNodeId(), nodeId, 2, UK::none);
    proc.connectBlocks(nodeId, proc.getAudioOutputNodeId(), 2, UK::none);

    // Single rebuild at the end
    proc.rebuildGraph();

    // Verify audio routes through the gain block
    juce::AudioBuffer<float> buf(2, kTotalSamples);
    generateSine(buf);
    juce::AudioBuffer<float> reference(buf);
    processInBlocks(proc, buf);

    if (!compareBuffers(buf, reference, 1e-5f, 0.5f))
    {
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- Multiple batched rebuilds in sequence ------------------------------------

static bool testMultipleBatchedRebuilds()
{
    printf("Test: multiple batched rebuilds in sequence... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    using UK = StellarrProcessor::UpdateKind;

    for (int i = 0; i < 10; ++i)
    {
        // Remove all non-IO nodes
        for (auto& node : proc.getGraph().getNodes())
        {
            auto nid = node->nodeID;
            if (nid != proc.getAudioInputNodeId() && nid != proc.getAudioOutputNodeId() &&
                nid != proc.getMidiInputNodeId() && nid != proc.getMidiOutputNodeId())
                proc.removeBlock(nid, UK::none);
        }

        proc.disconnectBlocks(proc.getAudioInputNodeId(), proc.getAudioOutputNodeId(), UK::none);

        float g = 0.1f * static_cast<float>(i + 1);
        auto gain = std::make_unique<stellarr::GainBlock>();
        gain->setGain(g);
        auto nodeId = proc.addBlock(std::move(gain), UK::none);

        proc.connectBlocks(proc.getAudioInputNodeId(), nodeId, 2, UK::none);
        proc.connectBlocks(nodeId, proc.getAudioOutputNodeId(), 2, UK::none);

        proc.rebuildGraph();
    }

    // After 10 rebuilds, final gain should be 1.0
    juce::AudioBuffer<float> buf(2, kTotalSamples);
    generateSine(buf);
    juce::AudioBuffer<float> reference(buf);
    processInBlocks(proc, buf);

    if (!compareBuffers(buf, reference, 1e-5f, 1.0f))
    {
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- Rapid session restore ----------------------------------------------------

static const char* kSessionWithPluginBlock = R"({
    "version": 1,
    "grid": {"columns": 12, "rows": 6},
    "blocks": [
        {"id": "in1", "type": "input", "name": "Input", "col": 0, "row": 2},
        {
            "id": "p1", "type": "plugin", "name": "Plugin", "col": 5, "row": 2,
            "pluginId": "nonexistent-test-plugin",
            "pluginName": "Test Plugin",
            "states": [{"pluginState":"","mix":1.0,"balance":0.0,"level":0.0,"bypassed":false,"bypassMode":"thru"}],
            "activeStateIndex": 0
        },
        {"id": "out1", "type": "output", "name": "Output", "col": 11, "row": 2}
    ],
    "connections": [
        {"sourceId": "in1", "destId": "p1"},
        {"sourceId": "p1", "destId": "out1"}
    ],
    "scenes": [],
    "activeSceneIndex": -1
})";

static const char* kSessionPassthrough = R"({
    "version": 1,
    "grid": {"columns": 12, "rows": 6},
    "blocks": [
        {"id": "in1", "type": "input", "name": "Input", "col": 0, "row": 2},
        {"id": "out1", "type": "output", "name": "Output", "col": 11, "row": 2}
    ],
    "connections": [
        {"sourceId": "in1", "destId": "out1"}
    ],
    "scenes": [],
    "activeSceneIndex": -1
})";

static bool testRapidSessionRestore()
{
    printf("Test: rapid session restore does not crash... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    // Alternate between two sessions 20 times in rapid succession
    for (int i = 0; i < 20; ++i)
    {
        auto session = juce::JSON::parse(
            (i % 2 == 0) ? kSessionWithPluginBlock : kSessionPassthrough);
        bridge.restoreSession(session);
    }

    // Verify the graph is functional after all switches
    auto& map = PresetSwitchTestAccess::getBlockNodeMap(bridge);
    if (map.empty())
    {
        fprintf(stderr, "  no blocks after rapid restore\n");
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    // Process audio to confirm no crash
    juce::AudioBuffer<float> buf(2, kBlockSize);
    generateSine(buf);
    juce::MidiBuffer midi;
    proc.processBlock(buf, midi);

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- Session restore then passthrough -----------------------------------------

static bool testClearAndRebuildProducesWorkingGraph()
{
    printf("Test: clear graph + rebuild produces working graph... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    // Load a session with a (missing) plugin block
    bridge.restoreSession(juce::JSON::parse(kSessionWithPluginBlock));

    // Now restore a clean passthrough session
    bridge.restoreSession(juce::JSON::parse(kSessionPassthrough));

    // Verify audio passes through without crashing and produces non-silent output
    juce::AudioBuffer<float> buf(2, kTotalSamples);
    generateSine(buf);
    processInBlocks(proc, buf);

    float peak = 0.0f;
    for (int ch = 0; ch < buf.getNumChannels(); ++ch)
        peak = juce::jmax(peak, buf.getMagnitude(ch, 0, buf.getNumSamples()));

    if (peak < 0.01f)
    {
        fprintf(stderr, "  output is silent (peak %f)\n", static_cast<double>(peak));
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- pluginReady flag gates process -------------------------------------------

static bool testPluginReadyGatesProcess()
{
    printf("Test: PluginBlock with no plugin is safe to process... ");

    stellarr::PluginBlock block;
    block.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buf(2, kBlockSize);
    juce::MidiBuffer midi;

    // Fill with known value
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < kBlockSize; ++i)
            buf.setSample(ch, i, 0.5f);

    // Process with no plugin loaded (pluginReady=false)
    block.processBlock(buf, midi);

    // Buffer should be unchanged (process is a no-op)
    float val = buf.getSample(0, 0);
    if (std::abs(val - 0.5f) > 1e-6f)
    {
        fprintf(stderr, "  expected 0.5 after no-op process, got %f\n", static_cast<double>(val));
        printf("FAIL\n");
        block.releaseResources();
        return false;
    }

    block.releaseResources();
    printf("PASS\n");
    return true;
}

int main()
{
    int failures = 0;

    if (!testBatchedRebuildRoutesAudio())       ++failures;
    if (!testMultipleBatchedRebuilds())         ++failures;
    if (!testRapidSessionRestore())             ++failures;
    if (!testClearAndRebuildProducesWorkingGraph()) ++failures;
    if (!testPluginReadyGatesProcess())         ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
