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
  LifecyclePong: 'lifecycle/pong',                               // outbound (TS-only listener today)

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
