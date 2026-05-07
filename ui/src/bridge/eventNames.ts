// Event names used for the JUCE WebView <-> React UI bridge. Both sides
// of the wire (engine/bridge/EventNames.h + ui/src/bridge/eventNames.ts)
// MUST use the same strings -- keep this file in lock-step with
// EventNames.h whenever events are added, removed, or renamed.
//
// Convention: "domain/action" -- domain matches the bridge handler owning
// the event. Outbound events (engine -> UI) use past-tense actions
// (renamed, colorChanged, ...); inbound events (UI -> engine) use
// imperative actions (rename, setColor). The three "update/*" inbound
// names are kept verbatim because they already conform.
//
// Categories below mirror the dispatch table in StellarrBridge.cpp +
// every emit/emitSync call site across engine/bridge/*.cpp. See
// engine/bridge/EventNames.h for the matching C++ table.

export const EventNames = {
  // ---- Lifecycle (mixed direction) ----
  LifecycleBridgeReady: 'lifecycle/bridgeReady',                 // inbound
  LifecycleUiReady: 'lifecycle/uiReady',                         // inbound
  LifecycleScreenshotReady: 'lifecycle/screenshotReady',         // inbound
  LifecycleWelcome: 'lifecycle/welcome',                         // outbound
  LifecycleStartupProgress: 'lifecycle/startupProgress',         // outbound
  LifecycleStartupComplete: 'lifecycle/startupComplete',         // outbound
  LifecycleScreenshotSetup: 'lifecycle/screenshotSetup',         // outbound

  // ---- Block (inbound) ----
  BlockAdd: 'block/add',
  BlockRemove: 'block/remove',
  BlockMove: 'block/move',
  BlockRename: 'block/rename',
  BlockSetColor: 'block/setColor',
  BlockSetPlugin: 'block/setPlugin',
  BlockOpenEditor: 'block/openEditor',
  BlockCopy: 'block/copy',
  BlockPaste: 'block/paste',
  BlockSetMix: 'block/setMix',
  BlockSetBalance: 'block/setBalance',
  BlockSetLevel: 'block/setLevel',
  BlockToggleBypass: 'block/toggleBypass',
  BlockSetBypassMode: 'block/setBypassMode',

  // ---- Block (outbound) ----
  BlockAdded: 'block/added',
  BlockRemoved: 'block/removed',
  BlockMoved: 'block/moved',
  BlockRenamed: 'block/renamed',
  BlockColorChanged: 'block/colorChanged',
  BlockPluginSet: 'block/pluginSet',
  BlockCopied: 'block/copied',
  BlockMixChanged: 'block/mixChanged',
  BlockBalanceChanged: 'block/balanceChanged',
  BlockLevelChanged: 'block/levelChanged',
  BlockBypassChanged: 'block/bypassChanged',
  BlockBypassModeChanged: 'block/bypassModeChanged',

  // ---- Connection ----
  ConnectionAdd: 'connection/add',                               // inbound
  ConnectionRemove: 'connection/remove',                         // inbound
  ConnectionAdded: 'connection/added',                           // outbound
  ConnectionRemoved: 'connection/removed',                       // outbound

  // ---- BlockState ----
  BlockStateSave: 'blockState/save',                             // inbound
  BlockStateAdd: 'blockState/add',                               // inbound
  BlockStateRecall: 'blockState/recall',                         // inbound
  BlockStateDelete: 'blockState/delete',                         // inbound
  BlockStateChanged: 'blockState/changed',                       // outbound

  // ---- Graph ----
  GraphState: 'graph/state',                                     // outbound

  // ---- Grid ----
  GridSetSize: 'grid/setSize',                                   // inbound
  GridState: 'grid/state',                                       // outbound

  // ---- Scene ----
  SceneAdd: 'scene/add',                                         // inbound
  SceneRecall: 'scene/recall',                                   // inbound
  SceneSave: 'scene/save',                                       // inbound
  SceneRename: 'scene/rename',                                   // inbound
  SceneDelete: 'scene/delete',                                   // inbound
  ScenesChanged: 'scene/changed',                                // outbound

  // ---- Session ----
  SessionNew: 'session/new',                                     // inbound
  SessionSave: 'session/save',                                   // inbound
  SessionSaveQuiet: 'session/saveQuiet',                         // inbound
  SessionLoad: 'session/load',                                   // inbound
  SessionSaved: 'session/saved',                                 // outbound

  // ---- Preset ----
  PresetPickDir: 'preset/pickDir',                               // inbound
  PresetLoadByIndex: 'preset/loadByIndex',                       // inbound
  PresetRename: 'preset/rename',                                 // inbound
  PresetDelete: 'preset/delete',                                 // inbound
  PresetGetList: 'preset/getList',                               // inbound
  PresetListUpdated: 'preset/listUpdated',                       // outbound
  PresetLoadStarted: 'preset/loadStarted',                       // outbound
  PresetLoadFinished: 'preset/loadFinished',                     // outbound

  // ---- MIDI ----
  MidiAddMapping: 'midi/addMapping',                             // inbound
  MidiRemoveMapping: 'midi/removeMapping',                       // inbound
  MidiClearMappings: 'midi/clearMappings',                       // inbound
  MidiGetMappings: 'midi/getMappings',                           // inbound
  MidiStartLearn: 'midi/startLearn',                             // inbound
  MidiCancelLearn: 'midi/cancelLearn',                           // inbound
  MidiSetMonitorEnabled: 'midi/setMonitorEnabled',               // inbound
  MidiInjectCC: 'midi/injectCC',                                 // inbound
  MidiMappingsChanged: 'midi/mappingsChanged',                   // outbound
  MidiLearnComplete: 'midi/learnComplete',                       // outbound
  MidiMonitorData: 'midi/monitorData',                           // outbound

  // ---- Plugins ----
  PluginsScan: 'plugins/scan',                                   // inbound
  PluginsGetScanDirs: 'plugins/getScanDirs',                     // inbound
  PluginsPickScanDir: 'plugins/pickScanDir',                     // inbound
  PluginsRemoveScanDir: 'plugins/removeScanDir',                 // inbound
  PluginsScanStarted: 'plugins/scanStarted',                     // outbound
  PluginsListUpdated: 'plugins/listUpdated',                     // outbound
  PluginsScanDirsUpdated: 'plugins/scanDirsUpdated',             // outbound

  // ---- Settings ----
  // Cross-cutting user preferences persisted in ApplicationProperties.
  SettingsGetDeveloperMode: 'settings/getDeveloperMode',         // inbound
  SettingsSetDeveloperMode: 'settings/setDeveloperMode',         // inbound
  SettingsDeveloperModeState: 'settings/developerModeState',     // outbound

  // ---- Telemetry ----
  TelemetryGet: 'telemetry/get',                                 // inbound
  TelemetrySet: 'telemetry/set',                                 // inbound
  TelemetryState: 'telemetry/state',                             // outbound

  // ---- Tuner ----
  TunerSetEnabled: 'tuner/setEnabled',                           // inbound
  TunerGetReferencePitch: 'tuner/getReferencePitch',             // inbound
  TunerSetReferencePitch: 'tuner/setReferencePitch',             // inbound
  TunerData: 'tuner/data',                                       // outbound
  TunerReferencePitchState: 'tuner/referencePitchState',         // outbound

  // ---- Input block ----
  InputToggleTestTone: 'input/toggleTestTone',                   // inbound
  InputGetTestToneSamples: 'input/getTestToneSamples',           // inbound
  InputSetTestToneSample: 'input/setTestToneSample',             // inbound
  InputTestToneChanged: 'input/testToneChanged',                 // outbound
  InputTestToneSamplesUpdated: 'input/testToneSamplesUpdated',   // outbound
  InputTestToneSampleChanged: 'input/testToneSampleChanged',     // outbound

  // ---- Loudness ----
  LoudnessSetSelectedBlock: 'loudness/setSelectedBlock',         // inbound
  LoudnessSetTarget: 'loudness/setTarget',                       // inbound
  LoudnessSetWindow: 'loudness/setWindow',                       // inbound
  LoudnessWindowState: 'loudness/windowState',                   // outbound
  LoudnessBlockMetrics: 'loudness/blockMetrics',                 // outbound

  // ---- System ----
  // Cross-cutting application-level signals that don't belong to a single
  // domain handler.
  SystemStats: 'system/stats',                                   // outbound
  SystemAppConfig: 'system/appConfig',                           // outbound

  // ---- Update ----
  // Sparkle-driven update flow. The three inbound names already followed
  // the domain/action convention so are kept verbatim.
  UpdateCheck: 'update/check',                                   // inbound
  UpdateInstall: 'update/install',                               // inbound
  UpdateOpenReleaseNotes: 'update/open-release-notes',           // inbound
  UpdateState: 'update/state',                                   // outbound
} as const;

export type EventName = (typeof EventNames)[keyof typeof EventNames];

// Events the UI sends to the engine (UI -> engine direction). Used as
// the parameter type of sendEvent so a typo or wrong-direction usage
// fails at compile time. LifecycleBridgeReady and LifecycleScreenshotReady
// are bidirectional handshakes -- they also appear in OutboundEventName
// so listener registration for them stays valid.
export type InboundEventName =
  | typeof EventNames.LifecycleBridgeReady
  | typeof EventNames.LifecycleUiReady
  | typeof EventNames.LifecycleScreenshotReady
  // Block (imperative actions)
  | typeof EventNames.BlockAdd
  | typeof EventNames.BlockRemove
  | typeof EventNames.BlockMove
  | typeof EventNames.BlockRename
  | typeof EventNames.BlockSetColor
  | typeof EventNames.BlockSetPlugin
  | typeof EventNames.BlockOpenEditor
  | typeof EventNames.BlockCopy
  | typeof EventNames.BlockPaste
  | typeof EventNames.BlockSetMix
  | typeof EventNames.BlockSetBalance
  | typeof EventNames.BlockSetLevel
  | typeof EventNames.BlockToggleBypass
  | typeof EventNames.BlockSetBypassMode
  // Connection (imperative)
  | typeof EventNames.ConnectionAdd
  | typeof EventNames.ConnectionRemove
  // BlockState (imperative)
  | typeof EventNames.BlockStateSave
  | typeof EventNames.BlockStateAdd
  | typeof EventNames.BlockStateRecall
  | typeof EventNames.BlockStateDelete
  // Grid (imperative)
  | typeof EventNames.GridSetSize
  // Scene (imperative)
  | typeof EventNames.SceneAdd
  | typeof EventNames.SceneRecall
  | typeof EventNames.SceneSave
  | typeof EventNames.SceneRename
  | typeof EventNames.SceneDelete
  // Session (imperative)
  | typeof EventNames.SessionNew
  | typeof EventNames.SessionSave
  | typeof EventNames.SessionSaveQuiet
  | typeof EventNames.SessionLoad
  // Preset (imperative)
  | typeof EventNames.PresetPickDir
  | typeof EventNames.PresetLoadByIndex
  | typeof EventNames.PresetRename
  | typeof EventNames.PresetDelete
  | typeof EventNames.PresetGetList
  // MIDI (imperative)
  | typeof EventNames.MidiAddMapping
  | typeof EventNames.MidiRemoveMapping
  | typeof EventNames.MidiClearMappings
  | typeof EventNames.MidiGetMappings
  | typeof EventNames.MidiStartLearn
  | typeof EventNames.MidiCancelLearn
  | typeof EventNames.MidiSetMonitorEnabled
  | typeof EventNames.MidiInjectCC
  // Plugins (imperative)
  | typeof EventNames.PluginsScan
  | typeof EventNames.PluginsGetScanDirs
  | typeof EventNames.PluginsPickScanDir
  | typeof EventNames.PluginsRemoveScanDir
  // Settings (imperative)
  | typeof EventNames.SettingsGetDeveloperMode
  | typeof EventNames.SettingsSetDeveloperMode
  // Telemetry (imperative)
  | typeof EventNames.TelemetryGet
  | typeof EventNames.TelemetrySet
  // Tuner (imperative)
  | typeof EventNames.TunerSetEnabled
  | typeof EventNames.TunerGetReferencePitch
  | typeof EventNames.TunerSetReferencePitch
  // Input block (imperative)
  | typeof EventNames.InputToggleTestTone
  | typeof EventNames.InputGetTestToneSamples
  | typeof EventNames.InputSetTestToneSample
  // Loudness (imperative)
  | typeof EventNames.LoudnessSetSelectedBlock
  | typeof EventNames.LoudnessSetTarget
  | typeof EventNames.LoudnessSetWindow
  // Update (imperative; verbatim names predate the rename)
  | typeof EventNames.UpdateCheck
  | typeof EventNames.UpdateInstall
  | typeof EventNames.UpdateOpenReleaseNotes;

// Events the engine emits to the UI (engine -> UI direction). Used as
// the listener key type so registering a handler for an inbound-only
// name fails at compile time. LifecycleBridgeReady and
// LifecycleScreenshotReady appear here too because they are
// bidirectional handshakes.
export type OutboundEventName =
  | typeof EventNames.LifecycleBridgeReady
  | typeof EventNames.LifecycleScreenshotReady
  | typeof EventNames.LifecycleWelcome
  | typeof EventNames.LifecycleStartupProgress
  | typeof EventNames.LifecycleStartupComplete
  | typeof EventNames.LifecycleScreenshotSetup
  // Block (past-tense)
  | typeof EventNames.BlockAdded
  | typeof EventNames.BlockRemoved
  | typeof EventNames.BlockMoved
  | typeof EventNames.BlockRenamed
  | typeof EventNames.BlockColorChanged
  | typeof EventNames.BlockPluginSet
  | typeof EventNames.BlockCopied
  | typeof EventNames.BlockMixChanged
  | typeof EventNames.BlockBalanceChanged
  | typeof EventNames.BlockLevelChanged
  | typeof EventNames.BlockBypassChanged
  | typeof EventNames.BlockBypassModeChanged
  // Connection (past-tense)
  | typeof EventNames.ConnectionAdded
  | typeof EventNames.ConnectionRemoved
  // BlockState (past-tense)
  | typeof EventNames.BlockStateChanged
  // Graph (descriptive)
  | typeof EventNames.GraphState
  // Grid (descriptive)
  | typeof EventNames.GridState
  // Scene (past-tense)
  | typeof EventNames.ScenesChanged
  // Session (past-tense)
  | typeof EventNames.SessionSaved
  // Preset (descriptive / past-tense)
  | typeof EventNames.PresetListUpdated
  | typeof EventNames.PresetLoadStarted
  | typeof EventNames.PresetLoadFinished
  // MIDI (past-tense / descriptive)
  | typeof EventNames.MidiMappingsChanged
  | typeof EventNames.MidiLearnComplete
  | typeof EventNames.MidiMonitorData
  // Plugins (past-tense / descriptive)
  | typeof EventNames.PluginsScanStarted
  | typeof EventNames.PluginsListUpdated
  | typeof EventNames.PluginsScanDirsUpdated
  // Settings (descriptive)
  | typeof EventNames.SettingsDeveloperModeState
  // Telemetry (descriptive)
  | typeof EventNames.TelemetryState
  // Tuner (descriptive)
  | typeof EventNames.TunerData
  | typeof EventNames.TunerReferencePitchState
  // Input block (past-tense)
  | typeof EventNames.InputTestToneChanged
  | typeof EventNames.InputTestToneSamplesUpdated
  | typeof EventNames.InputTestToneSampleChanged
  // Loudness (descriptive)
  | typeof EventNames.LoudnessWindowState
  | typeof EventNames.LoudnessBlockMetrics
  // System (descriptive)
  | typeof EventNames.SystemStats
  | typeof EventNames.SystemAppConfig
  // Update (descriptive)
  | typeof EventNames.UpdateState;
