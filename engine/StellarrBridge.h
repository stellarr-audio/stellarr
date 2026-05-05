#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include "Telemetry.h"
#include "bridge/GraphHandler.h"
#include "bridge/IBridgeEmitter.h"
#include "bridge/InputBlockHandler.h"
#include "bridge/MidiHandler.h"
#include "bridge/ParamHandler.h"
#include "bridge/PresetHandler.h"
#include "bridge/SceneHandler.h"
#include "bridge/UpdateHandler.h"

class StellarrProcessor;
namespace stellarr { class Block; class PluginBlock; }

class StellarrBridge : public stellarr::bridge::IBridgeEmitter, private juce::AsyncUpdater
{
public:
    StellarrBridge();
    ~StellarrBridge() override;

    void setProcessor(StellarrProcessor* proc);
    void setAppProperties(juce::ApplicationProperties* props);
    juce::WebBrowserComponent::Options configureOptions(juce::WebBrowserComponent::Options options);
    void setWebView(juce::WebBrowserComponent* browser);

    // Backward-compatibility alias for tests + any in-flight reference. The
    // Scene struct itself is owned by stellarr::bridge::SceneHandler — this
    // alias just keeps the historical StellarrBridge::Scene name working.
    using Scene = stellarr::bridge::Scene;

    juce::var serialiseSession() const;
    // Begins an async cooperative preset load. Returns true if the load was
    // STARTED (lock acquired, session well-formed); returns false on early
    // rejection (malformed session or restoreMutex contention) — no events,
    // no callback, in that case.
    //
    // When true is returned, plugin instances are loaded one per
    // message-loop tick (via juce::AsyncUpdater) so the UI stays responsive.
    // The optional onComplete callback fires later on the message thread
    // with success=true if Phase 1+2 completed cleanly, or success=false if
    // anything threw mid-way. presetLoadStarted is emitted synchronously
    // before this call returns; presetLoadFinished is emitted just before
    // onComplete fires.
    bool restoreSession(juce::var session,
                        std::function<void(bool success)> onComplete = nullptr);
    void sendSystemStats(double cpuPercent, float outputPeakLinear);
    void sendBlockMetrics();
    void drainMidiEvents();
    void sendTunerData();
    void sendMidiMonitorData();
    bool isTunerActive() const { return input ? input->isTunerActive() : false; }
    void setOnUiReady(std::function<void()> callback) { onUiReady = std::move(callback); }

    // Test accessors — delegate to PresetHandler which now owns the
    // preset-tracking state. preset is emplaced iff processor != nullptr;
    // tests always set the processor before reading these so the optional
    // is always populated at the point of access.
    const juce::StringArray& getPresetFiles() const { return preset->getPresetFiles(); }
    int getCurrentPresetIndex() const { return preset->getCurrentPresetIndex(); }
    const juce::File& getLastPresetFile() const { return preset->getLastPresetFile(); }
    void setPresetDirectory(const juce::File& dir) { preset->setPresetDirectory(dir); }

    // Testing seam — intercept emit calls before they hit the WebView.
    // Fired synchronously on the calling thread; useful for asserting event
    // sequences without spinning up a real browser.
    using EmitInterceptor = std::function<void(const juce::String&, const juce::var&)>;
    void setEmitInterceptor(EmitInterceptor cb) { emitInterceptor = std::move(cb); }

private:
    friend class PresetFileTestAccess;
    friend class CopyPasteTestAccess;
    friend class SessionTestAccess;
    friend class PresetSwitchTestAccess;
    void handleEvent(const juce::String& eventName, const juce::var& payload);
    void handleBridgeReady();
    void sendStartupProgress(const juce::String& status, int progress);
    void sendWelcome();
    void sendGraphState();

    // Plugin management event handlers
    void handleScanPlugins();
    void handleGetScanDirectories();
    void handlePickScanDirectory();
    void handleRemoveScanDirectory(const juce::var& json);

    // Telemetry
    void handleGetTelemetryEnabled();
    void handleSetTelemetryEnabled(const juce::var& json);

    // Tuner settings
    void handleGetReferencePitch();
    void handleSetReferencePitch(const juce::var& json);

    // Screenshot automation
    void handleScreenshotSetup();
    void handleScreenshotReady();

    // Loudness metering
    void handleSetSelectedBlock(const juce::var& json);
    void handleSetTargetLufs(const juce::var& json);
    void handleSetLufsWindow(const juce::var& json);

    void sendPluginList();
    void sendScanDirectories();

    void clearGraph();

    // IBridgeEmitter overrides — declared private so existing intra-class
    // call sites continue to resolve to these direct member calls today.
    // Phase 7 commits 2-9 wrap each domain in a class taking IBridgeEmitter&
    // through its context, at which point sub-handlers route via the public
    // interface rather than reaching across friend access.
    void emit(const juce::String& eventName, juce::DynamicObject* detail) override;
    void emitSync(const juce::String& eventName, juce::DynamicObject* detail) override;

    // Dispatch table for handleEvent. Defined as a private nested type +
    // private static accessor so the table's lambdas have access to the
    // private handle* member functions without needing friend declarations.
    struct EventEntry
    {
        std::function<void(StellarrBridge&, const juce::var&)> handler;
        // True if this event must be dropped while a preset restore is in
        // flight. Covers events that mutate graph / scenes / MIDI mappings /
        // persisted state — anything Phase 2's clearGraph() would obliterate
        // mid-load.
        bool dropDuringRestore = false;
    };
    using EventTable = std::unordered_map<juce::String, EventEntry>;
    static const EventTable& eventTable();

    juce::WebBrowserComponent* webView = nullptr;
    StellarrProcessor* processor = nullptr;
    juce::ApplicationProperties* appProperties = nullptr;

    std::map<juce::String, juce::AudioProcessorGraph::NodeID> blockNodeMap;
    std::map<juce::String, std::pair<int, int>> blockPositions;
    juce::var clipboardJson;

    std::function<void()> onUiReady;

    juce::String selectedBlockId;
    juce::String lufsWindow { "shortTerm" }; // "shortTerm" or "momentary"

    // Guards restoreSession against concurrent / re-entrant invocations.
    // Acquired via std::try_to_lock — competing callers drop their request
    // rather than queue, so a rapid burst of preset swaps cannot pile up
    // graph rebuilds and crash the audio thread.
    std::mutex restoreMutex;

    // Atomic mirror of "a restore is currently in flight". Read BEFORE the
    // mutex try_lock so we can short-circuit same-thread reentrancy (e.g.
    // drainMidiEvents → onPresetChange → handleLoadPresetByIndex →
    // restoreSession arriving on the message thread between AsyncUpdater
    // ticks). std::mutex::try_lock from the owning thread is undefined
    // behaviour, so we cannot rely on the mutex alone for that case.
    // Set true after the mutex is acquired in restoreSession; cleared
    // before pendingRestore.reset() in the completion path.
    std::atomic<bool> restoreInProgress { false };

    // Async cooperative preset-load state machine ----------------------------

    // One pre-loaded plugin instance for a block in the incoming session.
    struct PluginPreload {
        juce::String blockId;
        juce::String pluginId;
        juce::String pluginName;
        std::unique_ptr<juce::AudioPluginInstance> instance;
    };

    // Per-load state owned only while a restore is in flight. The unique_lock
    // releases restoreMutex via destructor when this struct is reset, which
    // guarantees the lock is freed on completion AND on exception (RAII).
    struct PendingRestore {
        std::unique_lock<std::mutex> lock;
        juce::var session;
        std::vector<PluginPreload> preloads;
        size_t nextIndex = 0;
        std::function<void(bool)> onComplete;
        // Cached parse — blocks array and plugin block list. We compute the
        // ordered list of plugin blocks once at start so handleAsyncUpdate()
        // can index it directly without re-iterating the JSON each tick.
        std::vector<juce::var> pluginBlocks;
    };

    std::optional<PendingRestore> pendingRestore;

    // Continuations queued while another restore was in flight. Fired (in
    // order) on the message thread once pendingRestore is reset. Used by
    // callers (e.g. handleScreenshotSetup) that need to wait for a current
    // load to drain before performing their next step.
    std::vector<std::function<void()>> postRestoreContinuations;
public:
    // Schedule `cb` to run after the currently-in-flight restoreSession
    // completes. If no restore is in flight, `cb` runs synchronously before
    // this returns. Must be called on the message thread.
    void runWhenRestoreIdle(std::function<void()> cb);
private:

    // juce::AsyncUpdater override — called on the message thread for each
    // cooperative tick of an in-flight preset load.
    void handleAsyncUpdate() override;

    // Phase 2 — runs in the final tick once all plugin instances have been
    // pre-loaded. Performs the suspend/clear/install/rebuild sequence.
    // Returns true on success, false on caught exception. Does NOT emit
    // presetLoadFinished or fire the completion callback — caller (always
    // handleAsyncUpdate) does that after release.
    bool finishRestore();

    EmitInterceptor emitInterceptor;

    // -- Sub-handlers ---------------------------------------------------------
    // Declared LAST so reverse-destruction order destroys them FIRST, before
    // any of the bridge state their contexts hold references / `[this]`
    // captures into. Lambda captures (e.g. ParamHandlerContext callbacks) are
    // released safely without dangling-reference risk during teardown.
    //
    // Invariant: param.has_value() iff processor != nullptr. Constructed and
    // reset by setProcessor(). All call sites assume this — no per-call guard
    // needed because the dispatch table only fires after bridgeReady, which
    // arrives strictly after setProcessor() in normal app startup.

    // Software updates (Sparkle).
    stellarr::bridge::UpdateHandler update { stellarr::bridge::UpdateHandlerContext { *this } };

    // Block parameter / state / emit helpers. Constructed in setProcessor.
    // Other free-function handlers (Scene / Midi / Preset) reach the public
    // emit helpers via param->... while their wrap commits are pending.
    std::optional<stellarr::bridge::ParamHandler> param;

    // Block + connection CRUD, clipboard, plugin editor, block metadata.
    // Constructed in setProcessor. Other free-function handlers (Preset,
    // SessionSerializer) call into graph->handleAddBlock when seeding a
    // default input/output graph.
    std::optional<stellarr::bridge::GraphHandler> graph;

    // MIDI mapping CRUD, learn, monitor, and the MidiMapper callback wiring.
    // Constructed in setProcessor AFTER param + graph because its context
    // callbacks route through them. Other free-function handlers (Preset,
    // SessionSerializer) call midi->emitMidiMappings() to broadcast the
    // current mapping list after pruning or restore.
    std::optional<stellarr::bridge::MidiHandler> midi;

    // Scene CRUD + the cross-handler scene state (scenes vector,
    // activeSceneIndex). Constructed in setProcessor AFTER param + midi
    // because their context callbacks now route scene mutations through
    // scene->mirrorActiveStateInScene / shiftStateMappingsAfterDelete /
    // tryRecallSceneByIndex. SessionSerializer + PresetHandler reach scene
    // state via scene->getScenes() / setScenes() / captureActiveScene().
    std::optional<stellarr::bridge::SceneHandler> scene;

    // Test-tone toggle/sample-pick + tuner enable/disable. Owns the
    // tunerActive flag. Constructed in setProcessor AFTER scene. Cross-handler
    // reads go via input->isTunerActive() (forwarded by isTunerActive()).
    std::optional<stellarr::bridge::InputBlockHandler> input;

    // Preset / session CRUD + grid sizing. Owns presetDirectory / presetFiles /
    // currentPresetIndex / lastPresetFile / gridCols / gridRows (Phase 7 /
    // Commit 8). appProperties remains on StellarrBridge — PresetHandler
    // captures a reference to that pointer slot via its context. Constructed
    // last in setProcessor; SessionSerializer + the bridge start-up flow read
    // grid + preset bookkeeping via preset->getGridCols() etc.
    std::optional<stellarr::bridge::PresetHandler> preset;
};
