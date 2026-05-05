#pragma once

namespace stellarr::bridge::events
{
    // Event names used for the JUCE WebView <-> React UI bridge. Both sides
    // of the wire (engine/bridge/EventNames.h + ui/src/bridge/eventNames.ts)
    // MUST use the same strings -- keep this file in lock-step with
    // eventNames.ts whenever events are added, removed, or renamed.
    //
    // Convention: "domain/action" -- domain matches the bridge handler
    // owning the event (block, scene, preset, midi, ...). Outbound events
    // (engine -> UI) use past-tense actions (renamed, colorChanged, ...);
    // inbound events (UI -> engine) use imperative actions (rename,
    // setColor). The three "update/*" names are kept verbatim because they
    // already conform.
    //
    // Categories below mirror the dispatch table in StellarrBridge.cpp +
    // every emit/emitSync call site across engine/bridge/*.cpp. See
    // ui/src/bridge/eventNames.ts for the matching TS table.

    // ---- Lifecycle (mixed direction) ------------------------------------
    inline constexpr const char* LifecycleBridgeReady       = "lifecycle/bridgeReady";       // inbound
    inline constexpr const char* LifecycleUiReady           = "lifecycle/uiReady";           // inbound
    inline constexpr const char* LifecycleScreenshotReady   = "lifecycle/screenshotReady";   // inbound
    inline constexpr const char* LifecycleWelcome           = "lifecycle/welcome";           // outbound
    inline constexpr const char* LifecycleStartupProgress   = "lifecycle/startupProgress";   // outbound
    inline constexpr const char* LifecycleStartupComplete   = "lifecycle/startupComplete";   // outbound
    inline constexpr const char* LifecycleScreenshotSetup   = "lifecycle/screenshotSetup";   // outbound
    inline constexpr const char* LifecyclePong              = "lifecycle/pong";              // outbound (TS-only listener today)

    // ---- Block (inbound) ------------------------------------------------
    inline constexpr const char* BlockAdd                   = "block/add";
    inline constexpr const char* BlockRemove                = "block/remove";
    inline constexpr const char* BlockMove                  = "block/move";
    inline constexpr const char* BlockRename                = "block/rename";
    inline constexpr const char* BlockSetColor              = "block/setColor";
    inline constexpr const char* BlockSetPlugin             = "block/setPlugin";
    inline constexpr const char* BlockOpenEditor            = "block/openEditor";
    inline constexpr const char* BlockCopy                  = "block/copy";
    inline constexpr const char* BlockPaste                 = "block/paste";
    inline constexpr const char* BlockSetMix                = "block/setMix";
    inline constexpr const char* BlockSetBalance            = "block/setBalance";
    inline constexpr const char* BlockSetLevel              = "block/setLevel";
    inline constexpr const char* BlockToggleBypass          = "block/toggleBypass";
    inline constexpr const char* BlockSetBypassMode         = "block/setBypassMode";

    // ---- Block (outbound) -----------------------------------------------
    inline constexpr const char* BlockAdded                 = "block/added";
    inline constexpr const char* BlockRemoved               = "block/removed";
    inline constexpr const char* BlockMoved                 = "block/moved";
    inline constexpr const char* BlockRenamed               = "block/renamed";
    inline constexpr const char* BlockColorChanged          = "block/colorChanged";
    inline constexpr const char* BlockPluginSet             = "block/pluginSet";
    inline constexpr const char* BlockCopied                = "block/copied";
    inline constexpr const char* BlockMixChanged            = "block/mixChanged";
    inline constexpr const char* BlockBalanceChanged        = "block/balanceChanged";
    inline constexpr const char* BlockLevelChanged          = "block/levelChanged";
    inline constexpr const char* BlockBypassChanged         = "block/bypassChanged";
    inline constexpr const char* BlockBypassModeChanged     = "block/bypassModeChanged";

    // ---- Connection (inbound + outbound) --------------------------------
    inline constexpr const char* ConnectionAdd              = "connection/add";              // inbound
    inline constexpr const char* ConnectionRemove           = "connection/remove";           // inbound
    inline constexpr const char* ConnectionAdded            = "connection/added";            // outbound
    inline constexpr const char* ConnectionRemoved          = "connection/removed";          // outbound

    // ---- BlockState (inbound + outbound) --------------------------------
    inline constexpr const char* BlockStateSave             = "blockState/save";             // inbound
    inline constexpr const char* BlockStateAdd              = "blockState/add";              // inbound
    inline constexpr const char* BlockStateRecall           = "blockState/recall";           // inbound
    inline constexpr const char* BlockStateDelete           = "blockState/delete";           // inbound
    inline constexpr const char* BlockStateChanged          = "blockState/changed";          // outbound (was blockStatesChanged)

    // ---- Graph (outbound) -----------------------------------------------
    inline constexpr const char* GraphState                 = "graph/state";

    // ---- Grid (inbound + outbound) --------------------------------------
    inline constexpr const char* GridSetSize                = "grid/setSize";                // inbound
    inline constexpr const char* GridState                  = "grid/state";                  // outbound

    // ---- Scene (inbound + outbound) -------------------------------------
    inline constexpr const char* SceneAdd                   = "scene/add";                   // inbound
    inline constexpr const char* SceneRecall                = "scene/recall";                // inbound
    inline constexpr const char* SceneSave                  = "scene/save";                  // inbound
    inline constexpr const char* SceneRename                = "scene/rename";                // inbound
    inline constexpr const char* SceneDelete                = "scene/delete";                // inbound
    inline constexpr const char* ScenesChanged              = "scene/changed";               // outbound (was scenesChanged)

    // ---- Session (inbound + outbound) -----------------------------------
    inline constexpr const char* SessionNew                 = "session/new";                 // inbound
    inline constexpr const char* SessionSave                = "session/save";                // inbound
    inline constexpr const char* SessionSaveQuiet           = "session/saveQuiet";           // inbound
    inline constexpr const char* SessionLoad                = "session/load";                // inbound
    inline constexpr const char* SessionSaved               = "session/saved";               // outbound

    // ---- Preset (inbound + outbound) ------------------------------------
    inline constexpr const char* PresetPickDir              = "preset/pickDir";              // inbound
    inline constexpr const char* PresetLoadByIndex          = "preset/loadByIndex";          // inbound
    inline constexpr const char* PresetRename               = "preset/rename";               // inbound
    inline constexpr const char* PresetDelete               = "preset/delete";               // inbound
    inline constexpr const char* PresetGetList              = "preset/getList";              // inbound
    inline constexpr const char* PresetListUpdated          = "preset/listUpdated";          // outbound
    inline constexpr const char* PresetLoadStarted          = "preset/loadStarted";          // outbound
    inline constexpr const char* PresetLoadFinished         = "preset/loadFinished";         // outbound

    // ---- MIDI (inbound + outbound) --------------------------------------
    inline constexpr const char* MidiAddMapping             = "midi/addMapping";             // inbound
    inline constexpr const char* MidiRemoveMapping          = "midi/removeMapping";          // inbound
    inline constexpr const char* MidiClearMappings          = "midi/clearMappings";          // inbound
    inline constexpr const char* MidiGetMappings            = "midi/getMappings";            // inbound
    inline constexpr const char* MidiStartLearn             = "midi/startLearn";             // inbound
    inline constexpr const char* MidiCancelLearn            = "midi/cancelLearn";            // inbound
    inline constexpr const char* MidiSetMonitorEnabled      = "midi/setMonitorEnabled";      // inbound
    inline constexpr const char* MidiInjectCC               = "midi/injectCC";               // inbound
    inline constexpr const char* MidiMappingsChanged        = "midi/mappingsChanged";        // outbound
    inline constexpr const char* MidiLearnComplete          = "midi/learnComplete";          // outbound
    inline constexpr const char* MidiMonitorData            = "midi/monitorData";            // outbound

    // ---- Plugins (inbound + outbound) -----------------------------------
    inline constexpr const char* PluginsScan                = "plugins/scan";                // inbound
    inline constexpr const char* PluginsGetScanDirs         = "plugins/getScanDirs";         // inbound
    inline constexpr const char* PluginsPickScanDir         = "plugins/pickScanDir";         // inbound
    inline constexpr const char* PluginsRemoveScanDir       = "plugins/removeScanDir";       // inbound
    inline constexpr const char* PluginsScanStarted         = "plugins/scanStarted";         // outbound
    inline constexpr const char* PluginsListUpdated         = "plugins/listUpdated";         // outbound
    inline constexpr const char* PluginsScanDirsUpdated     = "plugins/scanDirsUpdated";     // outbound (was scanDirectoriesUpdated)

    // ---- Telemetry (inbound + outbound) ---------------------------------
    inline constexpr const char* TelemetryGet               = "telemetry/get";               // inbound
    inline constexpr const char* TelemetrySet               = "telemetry/set";               // inbound
    inline constexpr const char* TelemetryState             = "telemetry/state";             // outbound

    // ---- Tuner (inbound + outbound) -------------------------------------
    inline constexpr const char* TunerSetEnabled            = "tuner/setEnabled";            // inbound
    inline constexpr const char* TunerGetReferencePitch     = "tuner/getReferencePitch";     // inbound
    inline constexpr const char* TunerSetReferencePitch     = "tuner/setReferencePitch";     // inbound
    inline constexpr const char* TunerData                  = "tuner/data";                  // outbound
    inline constexpr const char* TunerReferencePitchState   = "tuner/referencePitchState";   // outbound

    // ---- Input block (inbound + outbound) -------------------------------
    inline constexpr const char* InputToggleTestTone        = "input/toggleTestTone";        // inbound
    inline constexpr const char* InputGetTestToneSamples    = "input/getTestToneSamples";    // inbound
    inline constexpr const char* InputSetTestToneSample     = "input/setTestToneSample";     // inbound
    inline constexpr const char* InputTestToneChanged       = "input/testToneChanged";       // outbound
    inline constexpr const char* InputTestToneSamplesUpdated= "input/testToneSamplesUpdated";// outbound
    inline constexpr const char* InputTestToneSampleChanged = "input/testToneSampleChanged"; // outbound

    // ---- Loudness (inbound + outbound) ----------------------------------
    inline constexpr const char* LoudnessSetSelectedBlock   = "loudness/setSelectedBlock";   // inbound
    inline constexpr const char* LoudnessSetTarget          = "loudness/setTarget";          // inbound (was setTargetLufs)
    inline constexpr const char* LoudnessSetWindow          = "loudness/setWindow";          // inbound (was setLufsWindow)
    inline constexpr const char* LoudnessWindowState        = "loudness/windowState";        // outbound (was lufsWindowState)
    inline constexpr const char* LoudnessBlockMetrics       = "loudness/blockMetrics";       // outbound (was blockMetrics)

    // ---- System (outbound) ----------------------------------------------
    // Cross-cutting application-level signals that don't belong to a single
    // domain handler.
    inline constexpr const char* SystemStats                = "system/stats";                // outbound (was systemStats)
    inline constexpr const char* SystemAppConfig            = "system/appConfig";            // outbound (was appConfig)

    // ---- Update (inbound + outbound) ------------------------------------
    // Sparkle-driven update flow. The three inbound names already followed
    // the domain/action convention so are kept verbatim.
    inline constexpr const char* UpdateCheck                = "update/check";                // inbound
    inline constexpr const char* UpdateInstall              = "update/install";              // inbound
    inline constexpr const char* UpdateOpenReleaseNotes     = "update/open-release-notes";   // inbound
    inline constexpr const char* UpdateState                = "update/state";                // outbound (was updateState)

} // namespace stellarr::bridge::events
