#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/PluginBlock.h"
#include "../StellarrBridge.h"
#include <mutex>
#include <thread>
#include <vector>

class PresetSwitchTestAccess
{
public:
    static const auto& getBlockNodeMap(StellarrBridge& b) { return b.blockNodeMap; }
    static std::mutex& getRestoreMutex(StellarrBridge& b) { return b.restoreMutex; }
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

// -- Reentrant restoreSession is rejected by try-lock --------------------------

static bool testRestoreSessionTryLockBlocksReentrant()
{
    printf("Test: restoreSession try-lock rejects reentrant request... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    // Start from a clean known-good baseline
    bridge.restoreSession(juce::JSON::parse(kSessionPassthrough));
    auto baselineSize = PresetSwitchTestAccess::getBlockNodeMap(bridge).size();

    // Hold the restoreMutex from the test thread, then dispatch a competing
    // restoreSession on a separate thread. The competing thread's try_lock
    // must fail and the body must drop early — we wait on the thread's join
    // to confirm it returned without blocking. Calling try_lock on a mutex
    // already locked by the same thread is undefined behaviour for
    // std::mutex, hence the explicit second thread.
    auto& mutex = PresetSwitchTestAccess::getRestoreMutex(bridge);
    {
        std::unique_lock<std::mutex> heldLock(mutex);

        std::thread competing([&bridge]()
        {
            bridge.restoreSession(juce::JSON::parse(kSessionWithPluginBlock));
        });
        competing.join();

        auto duringLockSize = PresetSwitchTestAccess::getBlockNodeMap(bridge).size();
        if (duringLockSize != baselineSize)
        {
            fprintf(stderr, "  blockNodeMap mutated while restoreMutex was held (was %zu, now %zu)\n",
                    baselineSize, duringLockSize);
            printf("FAIL\n");
            proc.releaseResources();
            return false;
        }
    }

    // After releasing the lock, restoreSession should succeed normally.
    bridge.restoreSession(juce::JSON::parse(kSessionWithPluginBlock));
    auto afterUnlockSize = PresetSwitchTestAccess::getBlockNodeMap(bridge).size();
    if (afterUnlockSize == 0)
    {
        fprintf(stderr, "  restoreSession failed to populate graph after unlock\n");
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- restoreSession brackets work with started/finished events ----------------

static bool testRestoreSessionEmitsStartedFinished()
{
    printf("Test: restoreSession emits presetLoadStarted then presetLoadFinished... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    std::vector<juce::String> events;
    bridge.setEmitInterceptor([&events](const juce::String& name, const juce::var&)
    {
        events.push_back(name);
    });

    bridge.restoreSession(juce::JSON::parse(kSessionPassthrough));

    int startedAt = -1;
    int finishedAt = -1;
    for (int i = 0; i < static_cast<int>(events.size()); ++i)
    {
        if (events[static_cast<size_t>(i)] == "presetLoadStarted" && startedAt < 0)
            startedAt = i;
        else if (events[static_cast<size_t>(i)] == "presetLoadFinished")
            finishedAt = i;
    }

    bridge.setEmitInterceptor(nullptr);

    if (startedAt < 0 || finishedAt < 0 || startedAt >= finishedAt)
    {
        fprintf(stderr, "  expected presetLoadStarted before presetLoadFinished (started=%d, finished=%d)\n",
                startedAt, finishedAt);
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
    printf("PASS\n");
    return true;
}

// -- restoreSession still emits presetLoadFinished when body throws -----------

static bool testRestoreSessionEmitsFinishedOnException()
{
    printf("Test: restoreSession emits presetLoadFinished even when body throws... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    std::vector<juce::String> events;
    // Let presetLoadStarted (which is emitted before the try block) succeed,
    // then throw from inside the try-protected body so the catch path runs
    // and emits presetLoadFinished before re-throwing. Capture every emit,
    // including ones triggered by the catch path.
    bool hasThrown = false;
    bridge.setEmitInterceptor([&events, &hasThrown](const juce::String& name, const juce::var&)
    {
        events.push_back(name);
        if (!hasThrown && name != "presetLoadStarted" && name != "presetLoadFinished")
        {
            hasThrown = true;
            throw std::runtime_error("test-injected failure during restoreSession body");
        }
    });

    bool caught = false;
    try
    {
        bridge.restoreSession(juce::JSON::parse(kSessionPassthrough));
    }
    catch (const std::runtime_error&)
    {
        caught = true;
    }

    bridge.setEmitInterceptor(nullptr);

    if (!caught)
    {
        fprintf(stderr, "  expected restoreSession to re-throw the injected exception\n");
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    bool sawStarted = false;
    bool sawFinished = false;
    for (auto& name : events)
    {
        if (name == "presetLoadStarted") sawStarted = true;
        else if (name == "presetLoadFinished") sawFinished = true;
    }

    if (!sawStarted || !sawFinished)
    {
        fprintf(stderr, "  expected both started and finished emits (started=%d, finished=%d)\n",
                static_cast<int>(sawStarted), static_cast<int>(sawFinished));
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    proc.releaseResources();
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
    if (!testRestoreSessionTryLockBlocksReentrant()) ++failures;
    if (!testRestoreSessionEmitsStartedFinished())   ++failures;
    if (!testRestoreSessionEmitsFinishedOnException()) ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
