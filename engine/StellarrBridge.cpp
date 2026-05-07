#include "StellarrBridge.h"
#include "StellarrProcessor.h"
#include "Telemetry.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/PluginBlock.h"
#include "bridge/EventNames.h"
#include "bridge/internal/BlockLookup.h"
#include <cmath>
#include <limits>
#include <optional>

using namespace stellarr::bridge::internal;
namespace events = stellarr::bridge::events;

StellarrBridge::StellarrBridge() = default;
StellarrBridge::~StellarrBridge() = default;

// -- Public state machine wrappers -------------------------------------------
// Thin trampoline into SessionSerializer::restoreSession. The has_value()
// guard keeps this safe to call before setProcessor (e.g. during shutdown
// teardown) — matches the behaviour of the old "if (processor == nullptr)
// return false" guard.

bool StellarrBridge::restoreSession(juce::var session,
                                    std::function<void(bool)> onComplete)
{
    if (!sessionSerializer.has_value()) return false;
    return sessionSerializer->restoreSession(std::move(session), std::move(onComplete));
}

void StellarrBridge::setProcessor(StellarrProcessor* proc)
{
    processor = proc;

    if (proc != nullptr)
    {
        param.emplace(stellarr::bridge::ParamHandlerContext {
            *proc,
            blockNodeMap,
            *this,
            // onBlockStateDeleted: shift per-scene + MIDI mapping bookkeeping
            // when a block state is removed. Mirrors the index shift that
            // PluginBlock::deleteState() already applied to its own active
            // index.
            [this](const juce::String& blockId, int deletedIndex)
            {
                if (processor == nullptr) return;

                auto* node = blockNodeMap.count(blockId)
                    ? processor->getGraph().getNodeForId(blockNodeMap.at(blockId))
                    : nullptr;
                auto* pb = node ? dynamic_cast<stellarr::PluginBlock*>(node->getProcessor())
                                : nullptr;
                const int newCount = pb != nullptr ? pb->getNumStates() : 0;

                scene->shiftStateMappingsAfterDelete(blockId, deletedIndex, newCount);

                processor->getMidiMapper().removeMappingsForBlockState(blockId, deletedIndex);
                midi->emitMidiMappings();
            },
            // onActiveStateChanged: sync the active scene's blockStateMap so
            // the rewire-dot prediction in the scene dropdown reflects the
            // new state, then re-emit scenes.
            [this](const juce::String& blockId, int newActiveIndex)
            {
                scene->mirrorActiveStateInScene(blockId, newActiveIndex);
            }
        });

        graph.emplace(stellarr::bridge::GraphHandlerContext {
            *proc,
            blockNodeMap,
            blockPositions,
            clipboardJson,
            *this,
            // markBlockDirty: route bypass-toggle dirty notifications through
            // ParamHandler. Safe to dereference param unconditionally — both
            // handlers are constructed in this same branch and the invariant
            // (param.has_value() iff processor != nullptr) holds.
            [this](const juce::String& blockId)
            {
                param->markDirtyAndEmit(blockId);
            },
            // emitMidiMappings / sendGraphState: thin trampolines into the
            // matching broadcast helpers. emitMidiMappings now routes through
            // MidiHandler; sendGraphState still lives on StellarrBridge while
            // its domain awaits a later Phase 7 commit.
            [this]() { midi->emitMidiMappings(); },
            [this]() { sendGraphState(); }
        });

        midi.emplace(stellarr::bridge::MidiHandlerContext {
            *proc,
            blockNodeMap,
            *this,
            // isRestoring: every MidiMapper callback that mutates per-preset
            // state checks this and skips the mutation while a restore is in
            // flight. Mirrors the drop-during-restore gate at the top of
            // handleEvent for WebView events. Routes through SessionSerializer
            // (Commit 9) — sessionSerializer is emplaced AFTER midi in this
            // same branch, so the optional may not yet hold a value when this
            // lambda is constructed; the runtime invocation always happens
            // after setProcessor returns.
            [this]() { return sessionSerializer.has_value() && sessionSerializer->isRestoring(); },
            // markBlockDirty: route bypass / mix / balance / level CC mutations
            // through ParamHandler. Same invariant as graph's callback.
            [this](const juce::String& blockId)
            {
                param->markDirtyAndEmit(blockId);
            },
            // emitBlockParams / emitBlockStates: forward to ParamHandler so a
            // CC-driven state recall re-broadcasts the same payload set the
            // UI-driven handleBlockStateEvent("recall") emits.
            [this](const juce::String& blockId, stellarr::PluginBlock* pb)
            {
                param->emitBlockParams(blockId, pb);
            },
            [this](const juce::String& blockId, stellarr::PluginBlock* pb)
            {
                param->emitBlockStates(blockId, pb);
            },
            // mirrorActiveStateInScene: sync the active scene's blockStateMap
            // so the rewire-dot prediction in the scene dropdown reflects the
            // CC-driven state change, then re-emit scenes.
            [this](const juce::String& blockId, int newActiveIndex)
            {
                scene->mirrorActiveStateInScene(blockId, newActiveIndex);
            },
            // setTunerActiveOnAllBlocks: delegate to InputBlockHandler which
            // owns the tunerActive flag and the per-block propagation loop.
            [this](bool enabled)
            {
                input->setTunerEnabledOnAllBlocks(enabled);
            },
            // loadPresetByIndex: drive the bridge's preset-switch flow from a
            // CC mapping. handleLoadPresetByIndex is itself try-locked, so a
            // competing call during an in-flight restore self-rejects.
            [this](int index)
            {
                auto json = juce::JSON::parse("{\"index\":" + juce::String(index) + "}");
                preset->handleLoadPresetByIndex(json);
            },
            // recallSceneByIndex: drive scene recall from a CC mapping.
            [this](int index)
            {
                scene->tryRecallSceneByIndex(index);
            }
        });

        scene.emplace(stellarr::bridge::SceneHandlerContext {
            *proc,
            blockNodeMap,
            *this,
            // emitBlockParams / emitBlockStates: forward to ParamHandler so
            // post-recall snapshots use the same payload set the UI-driven
            // handleBlockStateEvent("recall") emits. param has been emplaced
            // earlier in this branch; safe to dereference unconditionally.
            [this](const juce::String& blockId, stellarr::PluginBlock* pb)
            {
                param->emitBlockParams(blockId, pb);
            },
            [this](const juce::String& blockId, stellarr::PluginBlock* pb)
            {
                param->emitBlockStates(blockId, pb);
            }
        });

        input.emplace(stellarr::bridge::InputBlockHandlerContext {
            *proc,
            blockNodeMap,
            *this,
            // isDeveloperModeEnabled: gates test-tone commands at the engine
            // boundary so non-picker call paths (screenshot lifecycle actions,
            // future MIDI bindings, anything else firing input/toggleTestTone
            // or input/setTestToneSample directly) still respect the
            // developer-mode preference.
            [this]() {
                return appProperties != nullptr
                    && appProperties->getUserSettings()->getBoolValue("developerModeEnabled", false);
            }
        });

        preset.emplace(stellarr::bridge::PresetHandlerContext {
            *proc,
            blockNodeMap,
            appProperties,
            *this,
            // clearGraph / serialiseSession / restoreSession now live on
            // SessionSerializer (Commit 9). Trampoline through it; the
            // sub-handler is emplaced AFTER preset in this branch, so the
            // lambdas dereference at call time, not at construction.
            // sendGraphState is still a member of StellarrBridge.
            [this]() { sessionSerializer->clearGraph(); },
            [this]() -> juce::var { return sessionSerializer->serialiseSession(); },
            [this](juce::var session, std::function<void(bool)> onComplete) -> bool
            {
                return sessionSerializer->restoreSession(std::move(session), std::move(onComplete));
            },
            [this]() { sendGraphState(); },
            // graphAddBlock: PresetHandler::handleNewSession seeds an empty
            // input/output graph through GraphHandler. graph has been
            // emplaced earlier in this branch; safe to dereference
            // unconditionally.
            [this](const juce::var& j) { graph->handleAddBlock(j); },
            // setScenes / setActiveSceneIndex: hand a freshly-captured
            // default scene to SceneHandler when starting a new session.
            [this](std::vector<stellarr::bridge::Scene> s)
            {
                scene->setScenes(std::move(s));
            },
            [this](int idx) { scene->setActiveSceneIndex(idx); },
            // clearAllDirtyStates: clear the dirty-state set after a save so
            // the dirty-dot UI matches the on-disk session.
            [this]() { param->clearAllDirtyStates(); },
            // emitMidiMappings: re-broadcast the (now empty) preset-level
            // MIDI mapping list after handleNewSession resets it.
            [this]() { midi->emitMidiMappings(); },
            // stopAllTestTones: stop any active test tone when developer mode
            // is disabled so tones do not play silently with no UI to stop them.
            [this]() { input->stopAllTestTones(); }
        });

        sessionSerializer.emplace(stellarr::bridge::SessionSerializerContext {
            *proc,
            blockNodeMap,
            blockPositions,
            *this,
            // Scene state — owned by SceneHandler since Commit 6.
            [this]() { return scene->getScenes(); },
            [this]() { return scene->getActiveSceneIndex(); },
            [this]() { scene->captureActiveScene(); },
            [this](std::vector<stellarr::bridge::Scene> s)
            {
                scene->setScenes(std::move(s));
            },
            [this](int idx) { scene->setActiveSceneIndex(idx); },
            // Grid state — owned by PresetHandler since Commit 8.
            [this]() { return preset->getGridCols(); },
            [this]() { return preset->getGridRows(); },
            [this](int c) { preset->setGridCols(c); },
            [this](int r) { preset->setGridRows(r); },
            // Cross-handler post-restore broadcasts. sendGraphState still
            // lives on StellarrBridge; emitGridState lives on PresetHandler;
            // emitMidiMappings lives on MidiHandler.
            [this]() { sendGraphState(); },
            [this]() { preset->emitGridState(); },
            [this]() { midi->emitMidiMappings(); }
        });
    }
    else
    {
        // Reverse construction order on teardown. SessionSerializer was
        // constructed last; reset it first so any in-flight pendingRestore
        // is destroyed (releases restoreMutex) before the rest of the
        // bridge unwinds and its lambda captures dangle.
        sessionSerializer.reset();
        preset.reset();
        input.reset();
        scene.reset();
        midi.reset();
        graph.reset();
        param.reset();
    }
}

void StellarrBridge::setAppProperties(juce::ApplicationProperties* props)
{
    appProperties = props;
}

juce::WebBrowserComponent::Options StellarrBridge::configureOptions(juce::WebBrowserComponent::Options options)
{
    return options.withNativeFunction("sendToNative",
        [this](const juce::Array<juce::var>& args,
               juce::WebBrowserComponent::NativeFunctionCompletion completion)
        {
            if (args.size() >= 2)
                handleEvent(args[0].toString(), args[1]);
            else if (args.size() == 1)
                handleEvent(args[0].toString(), {});

            completion(juce::var("ok"));
        });
}

void StellarrBridge::setWebView(juce::WebBrowserComponent* browser)
{
    webView = browser;
}

// -- Event dispatch -----------------------------------------------------------

const StellarrBridge::EventTable& StellarrBridge::eventTable()
{
    static const EventTable t = []
    {
        EventTable m;
        // Lifecycle ----------------------------------------------------
        m[events::LifecycleBridgeReady]      = { [](StellarrBridge& b, const juce::var&)   { b.handleBridgeReady(); }, false };
        m[events::LifecycleUiReady]          = { [](StellarrBridge& b, const juce::var&)   { if (b.onUiReady) b.onUiReady(); b.handleScreenshotSetup(); }, false };
        m[events::LifecycleScreenshotReady]  = { [](StellarrBridge& b, const juce::var&)   { b.handleScreenshotReady(); }, false };
        // Software updates (Sparkle) ----------------------------------
        m[events::UpdateCheck]               = { [](StellarrBridge& b, const juce::var&)   { b.update.handleCheck(); }, false };
        m[events::UpdateInstall]             = { [](StellarrBridge& b, const juce::var&)   { b.update.handleInstall(); }, false };
        m[events::UpdateOpenReleaseNotes]    = { [](StellarrBridge& b, const juce::var& j) { b.update.handleOpenReleaseNotes(j); }, false };
        // Graph --------------------------------------------------------
        m[events::BlockAdd]                  = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleAddBlock(j); }, true };
        m[events::BlockRemove]               = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleRemoveBlock(j); }, true };
        m[events::BlockMove]                 = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleMoveBlock(j); }, true };
        m[events::ConnectionAdd]             = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleAddConnection(j); }, true };
        m[events::ConnectionRemove]          = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleRemoveConnection(j); }, true };
        m[events::BlockSetPlugin]            = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleSetBlockPlugin(j); }, true };
        m[events::BlockOpenEditor]           = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleOpenPluginEditor(j); }, false };
        m[events::BlockCopy]                 = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleCopyBlock(j); }, true };
        m[events::BlockPaste]                = { [](StellarrBridge& b, const juce::var& j) { b.graph->handlePasteBlock(j); }, true };
        m[events::BlockRename]               = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleRenameBlock(j); }, true };
        m[events::BlockSetColor]             = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleSetBlockColor(j); }, true };
        // MIDI mappings ------------------------------------------------
        m[events::MidiAddMapping]            = { [](StellarrBridge& b, const juce::var& j) { b.midi->handleAddMidiMapping(j); }, true };
        m[events::MidiRemoveMapping]         = { [](StellarrBridge& b, const juce::var& j) { b.midi->handleRemoveMidiMapping(j); }, true };
        m[events::MidiClearMappings]         = { [](StellarrBridge& b, const juce::var&)   { b.midi->handleClearMidiMappings(); }, true };
        m[events::MidiGetMappings]           = { [](StellarrBridge& b, const juce::var&)   { b.midi->emitMidiMappings(); }, false };
        m[events::MidiStartLearn]            = { [](StellarrBridge& b, const juce::var& j) { b.midi->handleStartMidiLearn(j); }, true };
        m[events::MidiCancelLearn]           = { [](StellarrBridge& b, const juce::var&)   { b.midi->handleCancelMidiLearn(); }, true };
        m[events::MidiSetMonitorEnabled]     = { [](StellarrBridge& b, const juce::var& j) { b.midi->handleSetMidiMonitorEnabled(j); }, false };
        m[events::MidiInjectCC]              = { [](StellarrBridge& b, const juce::var& j) { b.midi->handleInjectMidiCC(j); }, false };
        // Plugin management -------------------------------------------
        m[events::PluginsScan]               = { [](StellarrBridge& b, const juce::var&)   { b.handleScanPlugins(); }, true };
        m[events::PluginsGetScanDirs]        = { [](StellarrBridge& b, const juce::var&)   { b.handleGetScanDirectories(); }, false };
        m[events::PluginsPickScanDir]        = { [](StellarrBridge& b, const juce::var&)   { b.handlePickScanDirectory(); }, true };
        m[events::PluginsRemoveScanDir]      = { [](StellarrBridge& b, const juce::var& j) { b.handleRemoveScanDirectory(j); }, true };
        // Settings -----------------------------------------------------
        // developerModeEnabled is a UI-only preference; it never mutates the
        // graph / scenes / MIDI mappings, so dropDuringRestore=false matches
        // the other read-only preference handlers (telemetry, tuner pitch).
        m[events::SettingsGetDeveloperMode]  = { [](StellarrBridge& b, const juce::var&)   { b.preset->handleGetDeveloperMode(); }, false };
        m[events::SettingsSetDeveloperMode]  = { [](StellarrBridge& b, const juce::var& j) { b.preset->handleSetDeveloperMode(j); }, false };
        // Telemetry ----------------------------------------------------
        m[events::TelemetryGet]              = { [](StellarrBridge& b, const juce::var&)   { b.handleGetTelemetryEnabled(); }, false };
        m[events::TelemetrySet]              = { [](StellarrBridge& b, const juce::var& j) { b.handleSetTelemetryEnabled(j); }, false };
        // Tuner settings ----------------------------------------------
        m[events::TunerGetReferencePitch]    = { [](StellarrBridge& b, const juce::var&)   { b.handleGetReferencePitch(); }, false };
        m[events::TunerSetReferencePitch]    = { [](StellarrBridge& b, const juce::var& j) { b.handleSetReferencePitch(j); }, false };
        // Presets ------------------------------------------------------
        m[events::SessionNew]                = { [](StellarrBridge& b, const juce::var&)   { b.preset->handleNewSession(); }, true };
        m[events::SessionSave]               = { [](StellarrBridge& b, const juce::var&)   { b.preset->handleSaveSession(); }, true };
        m[events::SessionSaveQuiet]          = { [](StellarrBridge& b, const juce::var&)   { b.preset->handleSaveSessionQuiet(); }, true };
        m[events::SessionLoad]               = { [](StellarrBridge& b, const juce::var&)   { b.preset->handleLoadSession(); }, false };
        m[events::PresetPickDir]             = { [](StellarrBridge& b, const juce::var&)   { b.preset->handlePickPresetDirectory(); }, true };
        m[events::PresetLoadByIndex]         = { [](StellarrBridge& b, const juce::var& j) { b.preset->handleLoadPresetByIndex(j); }, false };
        m[events::PresetRename]              = { [](StellarrBridge& b, const juce::var& j) { b.preset->handleRenamePreset(j); }, true };
        m[events::PresetDelete]              = { [](StellarrBridge& b, const juce::var& j) { b.preset->handleDeletePreset(j); }, true };
        m[events::PresetGetList]             = { [](StellarrBridge& b, const juce::var&)   { b.preset->handleGetPresetList(); }, false };
        m[events::GridSetSize]               = { [](StellarrBridge& b, const juce::var& j) { b.preset->handleSetGridSize(j); }, true };
        // Scenes -------------------------------------------------------
        m[events::SceneAdd]                  = { [](StellarrBridge& b, const juce::var&)   { b.scene->handleAddScene(); }, true };
        m[events::SceneRecall]               = { [](StellarrBridge& b, const juce::var& j) { b.scene->handleRecallScene(j); }, true };
        m[events::SceneSave]                 = { [](StellarrBridge& b, const juce::var& j) { b.scene->handleSaveScene(j); }, true };
        m[events::SceneRename]               = { [](StellarrBridge& b, const juce::var& j) { b.scene->handleRenameScene(j); }, true };
        m[events::SceneDelete]               = { [](StellarrBridge& b, const juce::var& j) { b.scene->handleDeleteScene(j); }, true };
        // Input block controls ----------------------------------------
        m[events::InputToggleTestTone]       = { [](StellarrBridge& b, const juce::var& j) { b.input->handleToggleTestTone(j); }, true };
        m[events::InputGetTestToneSamples]   = { [](StellarrBridge& b, const juce::var&)   { b.input->handleGetTestToneSamples(); }, false };
        m[events::InputSetTestToneSample]    = { [](StellarrBridge& b, const juce::var& j) { b.input->handleSetTestToneSample(j); }, true };
        m[events::TunerSetEnabled]           = { [](StellarrBridge& b, const juce::var& j) { b.input->handleSetTunerEnabled(j); }, true };
        // Block parameters --------------------------------------------
        m[events::BlockSetMix]               = { [](StellarrBridge& b, const juce::var& j) { b.param->handleSetBlockMix(j); }, true };
        m[events::BlockSetBalance]           = { [](StellarrBridge& b, const juce::var& j) { b.param->handleSetBlockBalance(j); }, true };
        m[events::BlockSetLevel]             = { [](StellarrBridge& b, const juce::var& j) { b.param->handleSetBlockLevel(j); }, true };
        m[events::BlockToggleBypass]         = { [](StellarrBridge& b, const juce::var& j) { b.graph->handleToggleBlockBypass(j); }, true };
        m[events::BlockSetBypassMode]        = { [](StellarrBridge& b, const juce::var& j) { b.param->handleSetBlockBypassMode(j); }, true };
        // Block states -------------------------------------------------
        m[events::BlockStateSave]            = { [](StellarrBridge& b, const juce::var& j) { b.param->handleBlockStateEvent(j, "save"); }, true };
        m[events::BlockStateAdd]             = { [](StellarrBridge& b, const juce::var& j) { b.param->handleBlockStateEvent(j, "add"); }, true };
        m[events::BlockStateRecall]          = { [](StellarrBridge& b, const juce::var& j) { b.param->handleBlockStateEvent(j, "recall"); }, true };
        m[events::BlockStateDelete]          = { [](StellarrBridge& b, const juce::var& j) { b.param->handleBlockStateEvent(j, "delete"); }, true };
        // Loudness metering -------------------------------------------
        m[events::LoudnessSetSelectedBlock]  = { [](StellarrBridge& b, const juce::var& j) { b.handleSetSelectedBlock(j); }, false };
        m[events::LoudnessSetTarget]         = { [](StellarrBridge& b, const juce::var& j) { b.handleSetTargetLufs(j); }, true };
        m[events::LoudnessSetWindow]         = { [](StellarrBridge& b, const juce::var& j) { b.handleSetLufsWindow(j); }, false };
        return m;
    }();
    return t;
}

void StellarrBridge::handleEvent(const juce::String& eventName, const juce::var& payload)
{
    auto json = payload.isString()
        ? juce::JSON::parse(payload.toString())
        : payload;

    auto it = eventTable().find(eventName);
    if (it == eventTable().end())
    {
        DBG("StellarrBridge: unknown event '" << eventName << "'");
        return;
    }

    // While an async restoreSession is in flight (Phase 1 yielding between
    // plugin loads), drop any event that would mutate the graph, scenes, MIDI
    // mappings, or persisted preset state. Phase 2 calls clearGraph() and
    // overwrites everything; letting those edits run during the load window
    // would silently lose them on the next async tick. Read-only events and
    // the lock-guarded preset-load events (loadPresetByIndex, loadSession —
    // both reject themselves via restoreSession's try_lock) pass through.
    if (sessionSerializer.has_value() && sessionSerializer->isRestoring()
        && it->second.dropDuringRestore)
    {
        DBG("StellarrBridge: dropping '" << eventName << "' while preset restore is in flight");
        return;
    }

    it->second.handler(*this, json);
}

// -- Lifted inline handlers (Phase 3) -----------------------------------------
// Bodies lifted character-for-character from the old if/else if chain. The
// `processor != nullptr` guards that used to live in the if-clause now sit at
// the top of each function. Phase 4 will move these into engine/bridge/*.cpp.

// -- Helpers ------------------------------------------------------------------

void StellarrBridge::emit(const juce::String& eventName, juce::DynamicObject* detail)
{
    // Wrap the raw DynamicObject* once so the ReferenceCountedObjectPtr inside
    // `data` keeps it alive across both the interceptor call and the async
    // WebView emit. Building two separate juce::var's around the same raw
    // pointer would let the first temporary's destructor delete the object
    // before the second wrapper takes ownership.
    auto data = juce::var(detail);

    if (emitInterceptor) emitInterceptor(eventName, data);

    if (webView == nullptr) return;

    auto eventId = juce::Identifier(eventName);

    juce::MessageManager::callAsync([this, eventId, data]()
    {
        if (webView != nullptr)
            webView->emitEventIfBrowserIsVisible(eventId, data);
    });
}

void StellarrBridge::emitSync(const juce::String& eventName, juce::DynamicObject* detail)
{
    // Same lifetime guard as emit.
    auto data = juce::var(detail);

    if (emitInterceptor) emitInterceptor(eventName, data);

    if (webView == nullptr) return;

    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto eventId = juce::Identifier(eventName);
    webView->emitEventIfBrowserIsVisible(eventId, data);
}

// -- Startup and state broadcast ----------------------------------------------

void StellarrBridge::sendStartupProgress(const juce::String& status, int progress)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("status", status);
    detail->setProperty("progress", progress);
    emit(events::LifecycleStartupProgress, detail);
}

void StellarrBridge::handleBridgeReady()
{
    sendStartupProgress("Connecting to engine...", 10);
    sendWelcome();

    auto* cfg = new juce::DynamicObject();
   #if STELLARR_IS_DEV
    cfg->setProperty("flavour", "dev");
   #else
    cfg->setProperty("flavour", "prod");
   #endif
    emit(events::SystemAppConfig, cfg);

    handleGetTelemetryEnabled();
    handleGetReferencePitch();
    if (preset.has_value()) preset->handleGetDeveloperMode();

    // Restore LUFS window from settings
    if (appProperties != nullptr)
    {
        auto savedWindow = appProperties->getUserSettings()->getValue("lufsWindow", "shortTerm");
        if (savedWindow != "momentary") savedWindow = "shortTerm";
        lufsWindow = savedWindow;

        auto* detail = new juce::DynamicObject();
        detail->setProperty("window", lufsWindow);
        emit(events::LoudnessWindowState, detail);
    }

    juce::MessageManager::callAsync([this]()
    {
        sendStartupProgress("Scanning plugin libraries...", 30);
        sendScanDirectories();

        juce::MessageManager::callAsync([this]()
        {
            if (processor != nullptr)
            {
                processor->getPluginManager().scanPlugins();
                sendPluginList();
            }

            juce::MessageManager::callAsync([this]()
            {
                sendStartupProgress("Restoring session...", 60);

                // Final-step lambda — runs after the optional preset restore
                // either succeeds, fails, or is skipped. Seeds an empty
                // input/output graph if nothing was restored, then completes
                // startup.
                auto finishStartup = [this](bool restored)
                {
                    if (!restored && blockNodeMap.empty())
                    {
                        auto inputJson = juce::JSON::parse(R"({"type":"input","col":0,"row":2})");
                        auto outputJson = juce::JSON::parse(R"({"type":"output","col":11,"row":2})");
                        graph->handleAddBlock(inputJson);
                        graph->handleAddBlock(outputJson);
                    }

                    sendGraphState();
                    preset->sendPresetList();

                    sendStartupProgress("Ready", 100);
                    emit(events::LifecycleStartupComplete, new juce::DynamicObject());
                };

                if (appProperties != nullptr)
                {
                    auto* settings = appProperties->getUserSettings();

                    // Restore global MIDI mappings
                    auto globalMidi = settings->getValue("globalMidiMappings", "");
                    if (globalMidi.isNotEmpty() && processor != nullptr)
                        processor->getMidiMapper().loadGlobalMappings(juce::JSON::parse(globalMidi));

                    auto savedDir = settings->getValue("lastPresetDirectory", "");
                    auto savedIndex = settings->getIntValue("lastPresetIndex", -1);
                    if (savedDir.isNotEmpty())
                    {
                        preset->setPresetDirectory(juce::File(savedDir));
                        preset->handleGetPresetList();
                        preset->setCurrentPresetIndex(savedIndex);
                    }

                    auto savedFile = settings->getValue("lastPresetFile", "");
                    if (savedFile.isNotEmpty())
                    {
                        auto file = juce::File(savedFile);
                        if (file.existsAsFile())
                        {
                            auto jsonStr = file.loadFileAsString();
                            auto session = juce::JSON::parse(jsonStr);
                            if (session.getDynamicObject() != nullptr)
                            {
                                bool started = restoreSession(session,
                                    [this, file, finishStartup](bool ok)
                                    {
                                        if (ok)
                                        {
                                            preset->setLastPresetFile(file);
                                            preset->setPresetDirectory(file.getParentDirectory());
                                            preset->handleGetPresetList();

                                            const auto& files = preset->getPresetFiles();
                                            for (int i = 0; i < files.size(); ++i)
                                            {
                                                if (files[i] == file.getFileName())
                                                {
                                                    preset->setCurrentPresetIndex(i);
                                                    break;
                                                }
                                            }
                                        }
                                        finishStartup(ok);
                                    });

                                if (started) return; // finishStartup runs in callback
                            }
                        }
                    }
                }

                // No restore was started — fall through to default seed.
                finishStartup(false);
            });
        });
    });
}

void StellarrBridge::sendWelcome()
{
    if (webView == nullptr) return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("message", "Stellarr C++ engine is running");
    webView->emitEventIfBrowserIsVisible(events::LifecycleWelcome, juce::var(detail));
}

void StellarrBridge::sendGraphState()
{
    if (webView == nullptr) return;

    juce::Array<juce::var> blocksArray;
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        auto* blockObj = new juce::DynamicObject();
        blockObj->setProperty("id", blockId);
        blockObj->setProperty("nodeId", static_cast<int>(nodeId.uid));

        auto posIt = blockPositions.find(blockId);
        if (posIt != blockPositions.end())
        {
            blockObj->setProperty("col", posIt->second.first);
            blockObj->setProperty("row", posIt->second.second);
        }

        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                blockObj->setProperty("type", stellarr::blockTypeToString(block->getBlockType()));
                blockObj->setProperty("name", block->getName());
                if (block->getDisplayName().isNotEmpty())
                    blockObj->setProperty("displayName", block->getDisplayName());
                if (block->getBlockColor().isNotEmpty())
                    blockObj->setProperty("blockColor", block->getBlockColor());

                if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                {
                    blockObj->setProperty("pluginId", pluginBlock->getPluginIdentifier());
                    blockObj->setProperty("pluginName", pluginBlock->getPluginName());
                    blockObj->setProperty("pluginFormat", pluginBlock->getPluginFormat());
                    if (pluginBlock->isPluginMissing())
                        blockObj->setProperty("pluginMissing", true);
                    blockObj->setProperty("numStates", pluginBlock->getNumStates());
                    blockObj->setProperty("activeStateIndex", pluginBlock->getActiveStateIndex());
                    juce::Array<juce::var> dirtyArr;
                    for (int d : pluginBlock->getDirtyStates())
                        dirtyArr.add(d);
                    blockObj->setProperty("dirtyStates", dirtyArr);
                }

                blockObj->setProperty("mix", static_cast<double>(block->getMix()));
                blockObj->setProperty("balance", static_cast<double>(block->getBalance()));
                blockObj->setProperty("level", static_cast<double>(block->getLevelDb()));
                blockObj->setProperty("bypassed", block->isBypassed());
                blockObj->setProperty("bypassMode", stellarr::bypassModeToString(block->getBypassMode()));
            }
        }

        blocksArray.add(juce::var(blockObj));
    }

    juce::Array<juce::var> connectionsArray;
    std::map<juce::uint32, juce::String> nodeToBlock;
    for (auto& [blockId, nodeId] : blockNodeMap)
        nodeToBlock[nodeId.uid] = blockId;

    for (auto& conn : processor->getGraph().getConnections())
    {
        if (conn.source.channelIndex != 0) continue;

        auto srcIt = nodeToBlock.find(conn.source.nodeID.uid);
        auto dstIt = nodeToBlock.find(conn.destination.nodeID.uid);
        if (srcIt == nodeToBlock.end() || dstIt == nodeToBlock.end()) continue;

        auto* connObj = new juce::DynamicObject();
        connObj->setProperty("sourceId", srcIt->second);
        connObj->setProperty("destId", dstIt->second);
        connectionsArray.add(juce::var(connObj));
    }

    auto* state = new juce::DynamicObject();
    state->setProperty("blocks", blocksArray);
    state->setProperty("connections", connectionsArray);
    emit(events::GraphState, state);
    scene->emitScenes();
}

// -- Screenshot automation ----------------------------------------------------

void StellarrBridge::handleScreenshotSetup()
{
    auto configFile = juce::File("/tmp/stellarr-screenshot-config.json");
    if (!configFile.existsAsFile()) return;

    // If a restore is already in flight (e.g. the startup last-session
    // restore is still loading plugins when uiReady fires), defer the entire
    // screenshot setup until that load completes — otherwise the capture
    // script's fixed timers can race the still-rebuilding graph. Leave the
    // config file in place so the deferred call can re-read it.
    if (sessionSerializer.has_value() && sessionSerializer->isRestoring())
    {
        sessionSerializer->runWhenRestoreIdle([this]() { handleScreenshotSetup(); });
        return;
    }

    auto screenshotConfig = juce::JSON::parse(configFile.loadFileAsString());
    configFile.deleteFile();

    auto* obj = screenshotConfig.getDynamicObject();
    if (obj == nullptr) return;

    // Load preset if specified. The screenshotSetup emit kicks off the UI's
    // capture timers, so it must NOT fire until any async preset restore has
    // finished — otherwise the screenshot script can race the still-loading
    // graph and capture a partial or stale state.
    auto presetPath = obj->getProperty("preset").toString();
    if (presetPath.isNotEmpty())
    {
        auto file = juce::File(presetPath);
        if (presetPath.startsWith("~"))
            file = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                       .getChildFile(presetPath.substring(2));

        if (file.existsAsFile())
        {
            auto jsonStr = file.loadFileAsString();
            auto session = juce::JSON::parse(jsonStr);
            if (session.getDynamicObject() != nullptr)
            {
                // Snapshot scene index up front — obj may not survive into
                // the async callback.
                std::optional<int> sceneIndex;
                if (obj->hasProperty("scene"))
                    sceneIndex = static_cast<int>(obj->getProperty("scene"));

                // Hold a ref-counted handle to the config so the JS-bound
                // payload survives across the async tick.
                juce::var configHandle = screenshotConfig;

                bool started = restoreSession(session,
                    [this, sceneIndex, configHandle](bool ok)
                {
                    if (ok)
                    {
                        sendGraphState();
                        if (sceneIndex.has_value() && scene.has_value())
                        {
                            auto sceneJson = juce::JSON::parse(
                                "{\"index\":" + juce::String(*sceneIndex) + "}");
                            scene->handleRecallScene(sceneJson);
                        }
                    }
                    if (auto* o = configHandle.getDynamicObject())
                        emit(events::LifecycleScreenshotSetup, o);
                });

                if (started) return; // emit is deferred to the callback
            }
        }
    }

    // No async restore in flight — emit synchronously as before.
    emit(events::LifecycleScreenshotSetup, obj);
}

void StellarrBridge::handleScreenshotReady()
{
    // Write signal file so the capture script knows we're ready
    juce::File("/tmp/stellarr-screenshot-ready").create();
}

// -- System stats and tuner ---------------------------------------------------

void StellarrBridge::sendSystemStats(double cpuPercent, float outputPeakLinear)
{
    auto peakDb = outputPeakLinear > 0.0001f
        ? 20.0f * std::log10(outputPeakLinear)
        : -60.0f;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("cpu", cpuPercent);
    detail->setProperty("outputLevelDb", static_cast<double>(peakDb));
    detail->setProperty("clipping", outputPeakLinear > 1.0f);
    emit(events::SystemStats, detail);
}

void StellarrBridge::sendTunerData()
{
    if (processor == nullptr || webView == nullptr) return;

    static const char* noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
            {
                if (!inputBlock->isTunerEnabled()) continue;

                auto noteIdx = inputBlock->getTunerNoteIndex();
                auto* detail = new juce::DynamicObject();
                detail->setProperty("note", noteIdx >= 0 && noteIdx < 12
                    ? juce::String(noteNames[noteIdx]) : juce::String());
                detail->setProperty("octave", inputBlock->getTunerOctave());
                detail->setProperty("cents", static_cast<double>(inputBlock->getTunerCents()));
                detail->setProperty("frequency", static_cast<double>(inputBlock->getTunerFrequency()));
                detail->setProperty("confidence", static_cast<double>(inputBlock->getTunerConfidence()));
                emit(events::TunerData, detail);
                return;
            }
        }
    }
}

void StellarrBridge::sendMidiMonitorData()
{
    if (processor == nullptr || webView == nullptr) return;

    auto events = processor->getMidiMapper().drainMonitorEvents();
    if (events.empty()) return;

    juce::Array<juce::var> arr;
    for (auto& evt : events)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("type", evt.type);
        obj->setProperty("channel", evt.channel);
        obj->setProperty("data1", evt.data1);
        obj->setProperty("data2", evt.data2);
        arr.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("events", arr);
    emit(events::MidiMonitorData, detail);
}

void StellarrBridge::drainMidiEvents()
{
    if (processor == nullptr) return;
    processor->getMidiMapper().drainOutboundEvents();
}

// -- Plugin management --------------------------------------------------------

void StellarrBridge::handleScanPlugins()
{
    if (processor == nullptr) return;

    emit(events::PluginsScanStarted, new juce::DynamicObject());

    // Run scan on a background thread to avoid freezing the UI.
    // sendPluginList must run on the message thread (bridge emission).
    auto* proc = processor;
    auto* self = this;
    std::thread([proc, self]() {
        proc->getPluginManager().scanPlugins();
        juce::MessageManager::callAsync([self]() {
            self->sendPluginList();
        });
    }).detach();
}

void StellarrBridge::handleGetScanDirectories()
{
    sendScanDirectories();
}

void StellarrBridge::handlePickScanDirectory()
{
    if (processor == nullptr) return;

    juce::MessageManager::callAsync([this]()
    {
        if (processor == nullptr) return;

        juce::FileChooser chooser("Select Plugin Directory");

        if (!chooser.browseForDirectory()) return;

        processor->getPluginManager().addScanDirectory(
            chooser.getResult().getFullPathName());
        sendScanDirectories();
    });
}

void StellarrBridge::handleRemoveScanDirectory(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto path = obj->getProperty("path").toString();
    processor->getPluginManager().removeScanDirectory(path);
    sendScanDirectories();
}

void StellarrBridge::sendPluginList()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> plugins;
    for (auto& desc : processor->getPluginManager().getKnownPlugins().getTypes())
    {
        if (desc.name == "Stellarr") continue;

        auto* p = new juce::DynamicObject();
        p->setProperty("id", desc.createIdentifierString());
        p->setProperty("name", desc.name);
        p->setProperty("manufacturer", desc.manufacturerName);
        p->setProperty("format", desc.pluginFormatName);
        plugins.add(juce::var(p));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("plugins", plugins);
    emit(events::PluginsListUpdated, detail);
}

void StellarrBridge::sendScanDirectories()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> dirs;
    for (auto& d : processor->getPluginManager().getScanDirectories())
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("path", d.path);
        obj->setProperty("isDefault", d.isDefault);
        dirs.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("directories", dirs);
    emit(events::PluginsScanDirsUpdated, detail);
}

// -- Telemetry ----------------------------------------------------------------

void StellarrBridge::handleGetTelemetryEnabled()
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("enabled", stellarr::Telemetry::isEnabled(appProperties));
    emit(events::TelemetryState, detail);
}

void StellarrBridge::handleSetTelemetryEnabled(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    bool enabled = static_cast<bool>(obj->getProperty("enabled"));
    stellarr::Telemetry::setEnabled(appProperties, enabled);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("enabled", enabled);
    emit(events::TelemetryState, detail);
}

// -- Tuner settings -----------------------------------------------------------

void StellarrBridge::handleGetReferencePitch()
{
    float hz = 440.0f;
    if (appProperties != nullptr)
    {
        auto* settings = appProperties->getUserSettings();
        if (settings != nullptr)
            hz = static_cast<float>(settings->getDoubleValue("referencePitch", 440.0));
    }

    // Apply stored value to all input blocks
    if (processor != nullptr)
    {
        for (auto& [blockId, nodeId] : blockNodeMap)
        {
            if (auto* node = processor->getGraph().getNodeForId(nodeId))
                if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
                    inputBlock->setReferencePitch(hz);
        }
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("hz", static_cast<double>(hz));
    emit(events::TunerReferencePitchState, detail);
}

void StellarrBridge::handleSetReferencePitch(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    float hz = static_cast<float>(obj->getProperty("hz"));
    if (hz < 420.0f || hz > 460.0f) return;

    // Persist
    if (appProperties != nullptr)
    {
        auto* settings = appProperties->getUserSettings();
        if (settings != nullptr)
            settings->setValue("referencePitch", static_cast<double>(hz));
    }

    // Apply to all input blocks
    if (processor != nullptr)
    {
        for (auto& [blockId, nodeId] : blockNodeMap)
        {
            if (auto* node = processor->getGraph().getNodeForId(nodeId))
                if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
                    inputBlock->setReferencePitch(hz);
        }
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("hz", static_cast<double>(hz));
    emit(events::TunerReferencePitchState, detail);
}

// -- Loudness metering --------------------------------------------------------

void StellarrBridge::handleSetSelectedBlock(const juce::var& json)
{
    auto newId = json.getProperty("blockId", "").toString();

    // Disable measurement on previously selected block (unless it's the Output)
    if (selectedBlockId.isNotEmpty() && selectedBlockId != newId)
    {
        if (auto* node = getNodeForBlockId(blockNodeMap, *processor, selectedBlockId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
                if (block->getBlockType() != stellarr::BlockType::output)
                    block->setMeasureLoudness(false);
        }
    }

    selectedBlockId = newId;

    // Enable measurement on the newly selected block
    if (selectedBlockId.isNotEmpty())
    {
        if (auto* node = getNodeForBlockId(blockNodeMap, *processor, selectedBlockId))
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
                block->setMeasureLoudness(true);
    }
}

void StellarrBridge::handleSetTargetLufs(const juce::var& json)
{
    auto blockId = json.getProperty("blockId", "").toString();
    auto value = json.getProperty("lufs", juce::var());

    auto* node = getNodeForBlockId(blockNodeMap, *processor, blockId);
    if (node == nullptr) return;

    auto* output = dynamic_cast<stellarr::OutputBlock*>(node->getProcessor());
    if (output == nullptr) return;

    if (value.isVoid() || value.isUndefined() || value.isString())
        output->setTargetLufs(std::numeric_limits<float>::quiet_NaN());
    else
        output->setTargetLufs(static_cast<float>(static_cast<double>(value)));
}

void StellarrBridge::handleSetLufsWindow(const juce::var& json)
{
    auto window = json.getProperty("window", "shortTerm").toString();
    if (window != "momentary") window = "shortTerm";
    lufsWindow = window;

    if (appProperties != nullptr)
        appProperties->getUserSettings()->setValue("lufsWindow", window);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("window", lufsWindow);
    emit(events::LoudnessWindowState, detail);
}

void StellarrBridge::sendBlockMetrics()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> blocksArray;

    for (const auto& [blockId, nodeId] : blockNodeMap)
    {
        auto* node = processor->getGraph().getNodeForId(nodeId);
        if (node == nullptr) continue;
        auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor());
        if (block == nullptr) continue;
        if (! block->isMeasuringLoudness()) continue;

        const float lufs = (lufsWindow == "momentary")
            ? block->getMomentaryLufs()
            : block->getShortTermLufs();

        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", blockId);
        obj->setProperty("lufs", static_cast<double>(lufs));

        if (auto* output = dynamic_cast<stellarr::OutputBlock*>(node->getProcessor()))
        {
            if (output->hasTargetLufs())
                obj->setProperty("targetLufs", static_cast<double>(output->getTargetLufs()));
        }

        blocksArray.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blocks", blocksArray);
    detail->setProperty("window", lufsWindow);
    emit(events::LoudnessBlockMetrics, detail);
}
