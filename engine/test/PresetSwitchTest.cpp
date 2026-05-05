#include "TestUtils.h"
#include "blocks/GainBlock.h"
#include "blocks/PluginBlock.h"
#include "../StellarrBridge.h"
#include "../bridge/EventNames.h"
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace events = stellarr::bridge::events;

class PresetSwitchTestAccess
{
public:
    static const auto& getBlockNodeMap(StellarrBridge& b) { return b.blockNodeMap; }
    // Reaches the restoreMutex via SessionSerializer (Phase 7 / Commit 9).
    // This class is friended on stellarr::bridge::SessionSerializer so the
    // test thread can hold the mutex from the outside to assert competing
    // restoreSession calls drop on try_lock.
    static std::mutex& getRestoreMutex(StellarrBridge& b) { return b.sessionSerializer->restoreMutex; }
    static void loadPresetByIndex(StellarrBridge& b, const juce::var& j) { b.preset->handleLoadPresetByIndex(j); }
    static const juce::StringArray& getPresetFiles(StellarrBridge& b) { return b.preset->getPresetFiles(); }
    static void refreshPresetList(StellarrBridge& b) { b.preset->handleGetPresetList(); }
};

// Pump the JUCE message loop until `done` returns true or the timeout fires.
// Returns true if `done` flipped within the budget. Required because
// restoreSession now schedules its work through juce::AsyncUpdater, so the
// message thread must service the queue before the load completes.
static bool waitForRestoreCompletion(std::function<bool()> done, int timeoutMs = 5000)
{
    auto deadline = juce::Time::getMillisecondCounter() + static_cast<juce::uint32>(timeoutMs);
    while (!done() && juce::Time::getMillisecondCounter() < deadline)
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    return done();
}

// Synchronous restoreSession wrapper for tests — kicks off the async restore
// then drives the message loop until the completion callback fires. Returns
// the success flag from the callback, or `false` if the restore was rejected
// up front (no callback fires in that case).
static bool restoreSessionSync(StellarrBridge& bridge, const juce::var& session,
                               int timeoutMs = 5000)
{
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};
    bool started = bridge.restoreSession(session, [&](bool ok)
    {
        success.store(ok);
        done.store(true);
    });
    if (!started) return false;
    waitForRestoreCompletion([&]{ return done.load(); }, timeoutMs);
    return success.load();
}

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

    // Alternate between two sessions 20 times in rapid succession. With the
    // async cooperative restore, fire-and-forget calls now mostly get rejected
    // by the in-flight try-lock — pump the loop briefly between each so at
    // least some land. The point of the test is "does not crash", not
    // "every load succeeds".
    for (int i = 0; i < 20; ++i)
    {
        auto session = juce::JSON::parse(
            (i % 2 == 0) ? kSessionWithPluginBlock : kSessionPassthrough);
        bridge.restoreSession(session);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(5);
    }

    // Drain any in-flight restore so the assertions below see a stable graph.
    waitForRestoreCompletion([&]{
        // Best-effort: nothing surfaces a "is restoring" flag publicly, so
        // pump for a fixed budget. 200 ms is generous given no real plugins.
        return false;
    }, 200);

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
    restoreSessionSync(bridge, juce::JSON::parse(kSessionWithPluginBlock));

    // Now restore a clean passthrough session
    restoreSessionSync(bridge, juce::JSON::parse(kSessionPassthrough));

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
    restoreSessionSync(bridge, juce::JSON::parse(kSessionPassthrough));
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

        std::atomic<bool> startedFlag{true};
        std::thread competing([&bridge, &startedFlag]()
        {
            startedFlag.store(bridge.restoreSession(juce::JSON::parse(kSessionWithPluginBlock)));
        });
        competing.join();

        if (startedFlag.load())
        {
            fprintf(stderr, "  competing restoreSession was accepted despite lock contention\n");
            printf("FAIL\n");
            proc.releaseResources();
            return false;
        }

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
    restoreSessionSync(bridge, juce::JSON::parse(kSessionWithPluginBlock));
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

    std::vector<juce::String> recordedEvents;
    bridge.setEmitInterceptor([&recordedEvents](const juce::String& name, const juce::var&)
    {
        recordedEvents.push_back(name);
    });

    restoreSessionSync(bridge, juce::JSON::parse(kSessionPassthrough));

    int startedAt = -1;
    int finishedAt = -1;
    for (int i = 0; i < static_cast<int>(recordedEvents.size()); ++i)
    {
        if (recordedEvents[static_cast<size_t>(i)] == events::PresetLoadStarted && startedAt < 0)
            startedAt = i;
        else if (recordedEvents[static_cast<size_t>(i)] == events::PresetLoadFinished)
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
    printf("Test: restoreSession emits presetLoadFinished and reports failure when body throws... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    std::vector<juce::String> recordedEvents;
    // Throw from a non-bracket emit so it surfaces inside finishRestore on
    // the async update tick. The body's catch wraps that in success=false
    // and still emits presetLoadFinished.
    bool hasThrown = false;
    bridge.setEmitInterceptor([&recordedEvents, &hasThrown](const juce::String& name, const juce::var&)
    {
        recordedEvents.push_back(name);
        if (!hasThrown && name != events::PresetLoadStarted && name != events::PresetLoadFinished)
        {
            hasThrown = true;
            throw std::runtime_error("test-injected failure during restoreSession body");
        }
    });

    std::atomic<bool> done{false};
    std::atomic<bool> success{true}; // start true so we can see it flip
    bool started = bridge.restoreSession(juce::JSON::parse(kSessionPassthrough),
                                         [&](bool ok)
    {
        success.store(ok);
        done.store(true);
    });

    bool completed = started && waitForRestoreCompletion([&]{ return done.load(); });

    bridge.setEmitInterceptor(nullptr);

    if (!started)
    {
        fprintf(stderr, "  restoreSession refused to start\n");
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    if (!completed)
    {
        fprintf(stderr, "  restoreSession completion callback never fired\n");
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    if (success.load())
    {
        fprintf(stderr, "  expected callback success=false after thrown emit\n");
        printf("FAIL\n");
        proc.releaseResources();
        return false;
    }

    bool sawStarted = false;
    bool sawFinished = false;
    for (auto& name : recordedEvents)
    {
        if (name == events::PresetLoadStarted) sawStarted = true;
        else if (name == events::PresetLoadFinished) sawFinished = true;
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

// -- handleLoadPresetByIndex preserves bookkeeping when restore is rejected ---

static bool testRestoreSessionRejectionPreservesCallerBookkeeping()
{
    printf("Test: handleLoadPresetByIndex doesn't mutate state when restore is rejected... ");

    StellarrProcessor proc;
    proc.prepareToPlay(kSampleRate, kBlockSize);

    StellarrBridge bridge;
    bridge.setProcessor(&proc);

    // Stage two preset files on disk so handleLoadPresetByIndex has something
    // to read. Contents don't matter — we never let restoreSession run to
    // completion in the racing thread.
    auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                   .getChildFile("stellarr_test_preset_swap_"
                                 + juce::String(juce::Random::getSystemRandom().nextInt()));
    dir.createDirectory();
    auto fileA = dir.getChildFile("Preset_A.stellarr");
    auto fileB = dir.getChildFile("Preset_B.stellarr");
    fileA.replaceWithText(kSessionPassthrough);
    fileB.replaceWithText(kSessionWithPluginBlock);

    bridge.setPresetDirectory(dir);
    // Populate the in-memory file list from the directory we just staged so
    // handleLoadPresetByIndex sees both presets.
    PresetSwitchTestAccess::refreshPresetList(bridge);
    [[maybe_unused]] const auto& files = PresetSwitchTestAccess::getPresetFiles(bridge);

    // Load preset A so currentPresetIndex is established. handleLoadPresetByIndex
    // schedules the restore via AsyncUpdater — pump the loop until the
    // bookkeeping callback runs.
    PresetSwitchTestAccess::loadPresetByIndex(bridge, juce::JSON::parse(R"({"index":0})"));
    waitForRestoreCompletion([&]{ return bridge.getCurrentPresetIndex() == 0; });
    if (bridge.getCurrentPresetIndex() != 0)
    {
        fprintf(stderr, "  precondition failed: expected currentPresetIndex=0, got %d\n",
                bridge.getCurrentPresetIndex());
        printf("FAIL\n");
        dir.deleteRecursively();
        proc.releaseResources();
        return false;
    }
    auto baselineFile = bridge.getLastPresetFile().getFileName();

    // Hold the restoreMutex on the test thread, then ask a competing thread
    // to load preset B. The try_lock inside restoreSession must fail; the
    // caller must observe that and leave currentPresetIndex / lastPresetFile
    // untouched. (Same-thread try_lock on std::mutex is UB, hence the
    // separate thread.)
    auto& mutex = PresetSwitchTestAccess::getRestoreMutex(bridge);
    {
        std::unique_lock<std::mutex> heldLock(mutex);

        std::thread competing([&bridge]()
        {
            PresetSwitchTestAccess::loadPresetByIndex(bridge, juce::JSON::parse(R"({"index":1})"));
        });
        competing.join();

        if (bridge.getCurrentPresetIndex() != 0)
        {
            fprintf(stderr, "  currentPresetIndex mutated despite restore rejection (now %d)\n",
                    bridge.getCurrentPresetIndex());
            printf("FAIL\n");
            dir.deleteRecursively();
            proc.releaseResources();
            return false;
        }

        if (bridge.getLastPresetFile().getFileName() != baselineFile)
        {
            fprintf(stderr, "  lastPresetFile mutated despite restore rejection (now %s)\n",
                    bridge.getLastPresetFile().getFileName().toRawUTF8());
            printf("FAIL\n");
            dir.deleteRecursively();
            proc.releaseResources();
            return false;
        }
    }

    dir.deleteRecursively();
    proc.releaseResources();
    printf("PASS\n");
    return true;
}

int main()
{
    // restoreSession schedules its work through juce::AsyncUpdater, which
    // requires a live MessageManager. ScopedJuceInitialiser_GUI sets one up
    // for the duration of the test process.
    juce::ScopedJuceInitialiser_GUI juceInit;

    int failures = 0;

    if (!testBatchedRebuildRoutesAudio())       ++failures;
    if (!testMultipleBatchedRebuilds())         ++failures;
    if (!testRapidSessionRestore())             ++failures;
    if (!testClearAndRebuildProducesWorkingGraph()) ++failures;
    if (!testPluginReadyGatesProcess())         ++failures;
    if (!testRestoreSessionTryLockBlocksReentrant()) ++failures;
    if (!testRestoreSessionEmitsStartedFinished())   ++failures;
    if (!testRestoreSessionEmitsFinishedOnException()) ++failures;
    if (!testRestoreSessionRejectionPreservesCallerBookkeeping()) ++failures;

    printf("\n%d test(s) failed\n", failures);
    return failures;
}
