#pragma once
#include <atomic>
#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"
#include "Scene.h"

class StellarrProcessor;
class PresetSwitchTestAccess;

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the pattern
    // established by UpdateHandler / ParamHandler / GraphHandler / MidiHandler /
    // SceneHandler / InputBlockHandler / PresetHandler.
    //
    // SessionSerializer owns the rapid-preset-switch state machine — the
    // restoreMutex, the in-progress atomic, the pendingRestore optional, and
    // the post-restore continuation queue (Phase 7 / Commit 9). Cross-handler
    // reach (scene + grid state read/write, MIDI mapping rebroadcast, dirty
    // state clear, graph re-broadcast) is wrapped as std::function callbacks
    // so SessionSerializer stays unaware of the other handlers' types.
    struct SessionSerializerContext
    {
        StellarrProcessor& processor;
        BlockNodeMap& blockNodeMap;
        std::map<juce::String, std::pair<int, int>>& blockPositions;
        IBridgeEmitter& emit;

        // Scene state access (owned by SceneHandler since Commit 6).
        // captureActiveScene mirrors any unsaved tweaks made while the active
        // scene was current into its blockStateMap before serialise reads it.
        std::function<std::vector<Scene>()>             getScenes;
        std::function<int()>                            getActiveSceneIndex;
        std::function<void()>                           captureActiveScene;
        std::function<void(std::vector<Scene>)>         setScenes;
        std::function<void(int)>                        setActiveSceneIndex;

        // Grid state access (owned by PresetHandler since Commit 8).
        std::function<int()>                            getGridCols;
        std::function<int()>                            getGridRows;
        std::function<void(int)>                        setGridCols;
        std::function<void(int)>                        setGridRows;

        // Cross-handler operations fired during finishRestore() once the
        // suspended-window block install completes.
        std::function<void()>                           sendGraphState;
        std::function<void()>                           emitGridState;
        std::function<void()>                           emitMidiMappings;
    };

    // Bridge handler that owns the async cooperative preset-restore state
    // machine. Inherits juce::AsyncUpdater so handleAsyncUpdate() drains the
    // pre-load queue one plugin per message-loop tick (the rapid-preset-switch
    // crash fix from #132). All mutex / atomic / RAII cleanup behaviour is
    // preserved bit-for-bit from the previous StellarrBridge implementation.
    class SessionSerializer : public juce::AsyncUpdater
    {
    public:
        explicit SessionSerializer(SessionSerializerContext ctx);
        ~SessionSerializer() override;

        // Capture the current bridge + processor state as a juce::var (the
        // session JSON tree). Called from PresetHandler::handleSaveSession
        // and friends.
        juce::var serialiseSession() const;

        // Tear down every block + connection in the current graph. Called by
        // PresetHandler::handleNewSession and from finishRestore() inside the
        // suspended-window before the new graph is installed.
        void clearGraph();

        // Begins an async cooperative preset load. Returns true if the load
        // was STARTED (lock acquired, session well-formed); returns false on
        // early rejection (malformed session or restoreMutex contention) — no
        // events, no callback, in that case. Plugin instances are loaded one
        // per message-loop tick (via juce::AsyncUpdater) so the UI stays
        // responsive. The optional onComplete callback fires later on the
        // message thread with success=true if Phase 1+2 completed cleanly,
        // or success=false if anything threw mid-way. presetLoadStarted is
        // emitted synchronously before this call returns; presetLoadFinished
        // is emitted just before onComplete fires.
        bool restoreSession(juce::var session,
                            std::function<void(bool)> onComplete = nullptr);

        // Schedule `cb` to run after the currently-in-flight restoreSession
        // completes. If no restore is in flight, `cb` runs synchronously
        // before this returns. Must be called on the message thread.
        void runWhenRestoreIdle(std::function<void()> cb);

        // Read-only state probe used by handleEvent's drop-during-restore
        // gate, by handleScreenshotSetup, and by MidiHandler's isRestoring
        // callback. The atomic mirror is the canonical signal — it stays in
        // sync with pendingRestore.has_value() under the mutex.
        bool isRestoring() const { return pendingRestore.has_value(); }

    private:
        // PresetSwitchTest holds restoreMutex from a separate thread to
        // assert that competing restoreSession calls drop on try_lock. The
        // test reaches the mutex through its existing access class.
        friend class ::PresetSwitchTestAccess;

        // juce::AsyncUpdater override — message-thread cooperative tick.
        void handleAsyncUpdate() override;

        // Phase 2 — runs in the final tick once all plugin instances have
        // been pre-loaded. Performs the suspend/clear/install/rebuild
        // sequence. Returns true on success, false on caught exception.
        // Does NOT emit presetLoadFinished or fire the completion callback —
        // caller (always handleAsyncUpdate) does that after release.
        bool finishRestore();

        // One pre-loaded plugin instance for a block in the incoming session.
        struct PluginPreload {
            juce::String blockId;
            juce::String pluginId;
            juce::String pluginName;
            std::unique_ptr<juce::AudioPluginInstance> instance;
        };

        // Per-load state owned only while a restore is in flight. The
        // unique_lock releases restoreMutex via destructor when this struct
        // is reset, which guarantees the lock is freed on completion AND on
        // exception (RAII).
        struct PendingRestore {
            std::unique_lock<std::mutex> lock;
            juce::var session;
            std::vector<PluginPreload> preloads;
            size_t nextIndex = 0;
            std::function<void(bool)> onComplete;
            // Cached parse — blocks array and plugin block list. We compute
            // the ordered list of plugin blocks once at start so
            // handleAsyncUpdate() can index it directly without re-iterating
            // the JSON each tick.
            std::vector<juce::var> pluginBlocks;
        };

        SessionSerializerContext ctx;

        // Guards restoreSession against concurrent / re-entrant invocations.
        // Acquired via std::try_to_lock — competing callers drop their
        // request rather than queue, so a rapid burst of preset swaps cannot
        // pile up graph rebuilds and crash the audio thread.
        std::mutex restoreMutex;

        // Atomic mirror of "a restore is currently in flight". Read BEFORE
        // the mutex try_lock so we can short-circuit same-thread reentrancy
        // (e.g. drainMidiEvents → onPresetChange → handleLoadPresetByIndex →
        // restoreSession arriving on the message thread between AsyncUpdater
        // ticks). std::mutex::try_lock from the owning thread is undefined
        // behaviour, so we cannot rely on the mutex alone for that case.
        // Set true after the mutex is acquired in restoreSession; cleared
        // before pendingRestore.reset() in the completion path.
        std::atomic<bool> restoreInProgress { false };

        std::optional<PendingRestore> pendingRestore;

        // Continuations queued while another restore was in flight. Fired
        // (in order) on the message thread once pendingRestore is reset.
        // Used by callers (e.g. handleScreenshotSetup) that need to wait for
        // a current load to drain before performing their next step.
        std::vector<std::function<void()>> postRestoreContinuations;
    };
}
