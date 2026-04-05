import { create } from 'zustand';

export interface GridBlock {
  id: string;
  type: string;
  name: string;
  col: number;
  row: number;
  nodeId: number;
  testTone?: boolean;
  testToneSample?: string;
  pluginId?: string;
  pluginName?: string;
  pluginFormat?: string;
  displayName?: string;
  blockColor?: string;
  mix?: number;
  balance?: number;
  level?: number; // dB
  bypassed?: boolean;
  bypassMode?: string;
  hasEditor?: boolean;
  numStates?: number;
  activeStateIndex?: number;
  dirtyStates?: number[];
}

export interface Connection {
  sourceId: string;
  destId: string;
}

export interface GridSettings {
  columns: number;
  rows: number;
}

export interface ScanDirectory {
  path: string;
  isDefault: boolean;
}

export interface PluginInfo {
  id: string;
  name: string;
  manufacturer: string;
  format: string;
}

export interface MidiMapping {
  channel: number;
  cc: number;
  target: string;
  blockId?: string;
}

export interface MidiMonitorEvent {
  type: string;
  channel: number;
  data1: number;
  data2: number;
}

export interface Scene {
  name: string;
  blockStateMap: Record<string, number>;
}

interface StellarrState {
  loading: boolean;
  loadingStatus: string;
  loadingProgress: number;
  connected: boolean;
  activeTab: string;
  blocks: GridBlock[];
  connections: Connection[];
  grid: GridSettings;
  scanDirectories: ScanDirectory[];
  availablePlugins: PluginInfo[];
  presetDirectory: string;
  presetFiles: string[];
  currentPresetIndex: number;
  testToneSamples: string[];
  testToneSample: string;
  cpuPercent: number;
  memoryMB: number;
  justSaved: boolean;
  totalMemoryMB: number;
  scenes: Scene[];
  activeSceneIndex: number;
  midiMappings: MidiMapping[];
  midiLearning: boolean;
  midiMonitorEvents: MidiMonitorEvent[];
  midiMonitorEnabled: boolean;
  midiMappingActivity: Record<number, number>; // mapping index → timestamp of last activity

  setLoading: (loading: boolean) => void;
  setLoadingStatus: (status: string, progress: number) => void;
  setConnected: (value: boolean) => void;
  setActiveTab: (tab: string) => void;

  // Tuner
  tunerNote: string | null;
  tunerOctave: number;
  tunerCents: number;
  tunerFrequency: number;
  tunerConfidence: number;
  setTunerData: (
    note: string | null,
    octave: number,
    cents: number,
    frequency: number,
    confidence: number,
  ) => void;
  setBlockTestTone: (blockId: string, enabled: boolean) => void;
  setGridSize: (columns: number, rows: number) => void;
  setScanDirectories: (dirs: ScanDirectory[]) => void;
  setAvailablePlugins: (plugins: PluginInfo[]) => void;
  setBlockPlugin: (
    blockId: string,
    pluginId: string,
    pluginName: string,
    pluginFormat: string,
    hasEditor: boolean,
  ) => void;
  setBlockMix: (blockId: string, mix: number) => void;
  setBlockBalance: (blockId: string, balance: number) => void;
  setBlockLevel: (blockId: string, level: number) => void;
  setBlockDisplayName: (blockId: string, displayName: string) => void;
  setBlockColor: (blockId: string, blockColor: string) => void;
  setBlockBypassed: (blockId: string, bypassed: boolean) => void;
  setBlockBypassMode: (blockId: string, mode: string) => void;
  setBlockStates: (
    blockId: string,
    numStates: number,
    activeStateIndex: number,
    dirtyStates: number[],
  ) => void;
  setPresetList: (directory: string, files: string[], currentIndex: number) => void;
  setSystemStats: (cpu: number, memory: number, totalMemory: number) => void;
  setTestToneSamples: (samples: string[]) => void;
  setTestToneSample: (sample: string) => void;
  setJustSaved: (value: boolean) => void;
  setScenes: (scenes: Scene[], activeSceneIndex: number) => void;
  setMidiMappings: (mappings: MidiMapping[], learning: boolean) => void;
  appendMidiMonitorEvents: (events: MidiMonitorEvent[]) => void;
  clearMidiMonitor: () => void;
  setMidiMonitorEnabled: (enabled: boolean) => void;
  updateMidiActivity: (events: MidiMonitorEvent[]) => void;

  addBlock: (block: GridBlock) => void;
  removeBlock: (blockId: string) => void;
  moveBlock: (blockId: string, col: number, row: number) => void;
  addConnection: (conn: Connection) => void;
  removeConnection: (sourceId: string, destId: string) => void;
  syncGraph: (blocks: GridBlock[], connections: Connection[]) => void;

  clipboardBlockType: string | null;
  setClipboardBlockType: (type: string | null) => void;

  selectedBlockId: string | null;
  selectBlock: (blockId: string | null) => void;

  draggingConnection: {
    blockId: string;
    portType: 'input' | 'output';
    mouseX: number;
    mouseY: number;
  } | null;
  setDraggingConnection: (
    state: {
      blockId: string;
      portType: 'input' | 'output';
      mouseX: number;
      mouseY: number;
    } | null,
  ) => void;
}

export const useStore = create<StellarrState>((set) => ({
  loading: true,
  loadingStatus: 'Initialising...',
  loadingProgress: 0,
  connected: false,
  activeTab: 'grid',
  tunerNote: null,
  tunerOctave: 0,
  tunerCents: 0,
  tunerFrequency: 0,
  tunerConfidence: 0,
  blocks: [],
  connections: [],
  grid: { columns: 12, rows: 6 },
  scanDirectories: [],
  availablePlugins: [],
  presetDirectory: '',
  presetFiles: [],
  currentPresetIndex: -1,
  testToneSamples: [],
  testToneSample: 'Synth (Default)',
  cpuPercent: 0,
  memoryMB: 0,
  justSaved: false,
  totalMemoryMB: 1,
  scenes: [],
  activeSceneIndex: -1,
  midiMappings: [],
  midiLearning: false,
  midiMonitorEvents: [],
  midiMonitorEnabled: false,
  midiMappingActivity: {},
  clipboardBlockType: null,
  selectedBlockId: null,
  draggingConnection: null,

  setLoading: (loading) => set({ loading }),
  setLoadingStatus: (status, progress) => set({ loadingStatus: status, loadingProgress: progress }),
  setConnected: (value) => set({ connected: value }),
  setActiveTab: (tab) => set({ activeTab: tab }),

  setTunerData: (note, octave, cents, frequency, confidence) =>
    set({
      tunerNote: note,
      tunerOctave: octave,
      tunerCents: cents,
      tunerFrequency: frequency,
      tunerConfidence: confidence,
    }),

  setBlockTestTone: (blockId, enabled) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, testTone: enabled } : b)),
    })),

  setGridSize: (columns, rows) => set({ grid: { columns, rows } }),
  setScanDirectories: (dirs) => set({ scanDirectories: dirs }),
  setAvailablePlugins: (plugins) => set({ availablePlugins: plugins }),

  setBlockPlugin: (blockId, pluginId, pluginName, pluginFormat, hasEditor) =>
    set((s) => ({
      blocks: s.blocks.map((b) =>
        b.id === blockId ? { ...b, pluginId, pluginName, pluginFormat, hasEditor } : b,
      ),
    })),

  setBlockMix: (blockId, mix) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, mix } : b)),
    })),

  setBlockBalance: (blockId, balance) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, balance } : b)),
    })),

  setBlockLevel: (blockId, level) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, level } : b)),
    })),

  setBlockDisplayName: (blockId, displayName) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, displayName } : b)),
    })),

  setBlockColor: (blockId, blockColor) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, blockColor } : b)),
    })),

  setBlockBypassed: (blockId, bypassed) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, bypassed } : b)),
    })),

  setBlockBypassMode: (blockId, bypassMode) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, bypassMode } : b)),
    })),

  setBlockStates: (blockId, numStates, activeStateIndex, dirtyStates) =>
    set((s) => ({
      blocks: s.blocks.map((b) =>
        b.id === blockId ? { ...b, numStates, activeStateIndex, dirtyStates } : b,
      ),
    })),

  setPresetList: (directory, files, currentIndex) =>
    set({ presetDirectory: directory, presetFiles: files, currentPresetIndex: currentIndex }),

  setSystemStats: (cpu, memory, totalMemory) =>
    set({ cpuPercent: cpu, memoryMB: memory, totalMemoryMB: totalMemory }),

  setTestToneSamples: (samples) => set({ testToneSamples: samples }),
  setTestToneSample: (sample) => set({ testToneSample: sample }),
  setJustSaved: (value) => set({ justSaved: value }),

  setScenes: (scenes, activeSceneIndex) => set({ scenes, activeSceneIndex }),

  setMidiMappings: (mappings, learning) => set({ midiMappings: mappings, midiLearning: learning }),

  appendMidiMonitorEvents: (events) =>
    set((s) => ({
      midiMonitorEvents: [...s.midiMonitorEvents, ...events].slice(-200),
    })),

  clearMidiMonitor: () => set({ midiMonitorEvents: [] }),

  setMidiMonitorEnabled: (enabled) => set({ midiMonitorEnabled: enabled }),

  updateMidiActivity: (events) =>
    set((s) => {
      const activity = { ...s.midiMappingActivity };
      const now = Date.now();
      let changed = false;

      for (const evt of events) {
        for (let i = 0; i < s.midiMappings.length; i++) {
          const m = s.midiMappings[i];
          const channelMatch = m.channel === -1 || m.channel === evt.channel;
          if (!channelMatch) continue;

          if (evt.type === 'CC' && m.cc === evt.data1) {
            activity[i] = now;
            changed = true;
          } else if (evt.type === 'PC' && m.cc === -1 && m.target === 'presetChange') {
            activity[i] = now;
            changed = true;
          }
        }
      }

      return changed ? { midiMappingActivity: activity } : {};
    }),

  addBlock: (block) => set((s) => ({ blocks: [...s.blocks, block] })),

  removeBlock: (blockId) =>
    set((s) => ({
      blocks: s.blocks.filter((b) => b.id !== blockId),
      connections: s.connections.filter((c) => c.sourceId !== blockId && c.destId !== blockId),
      selectedBlockId: s.selectedBlockId === blockId ? null : s.selectedBlockId,
    })),

  moveBlock: (blockId, col, row) =>
    set((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, col, row } : b)),
    })),

  addConnection: (conn) => set((s) => ({ connections: [...s.connections, conn] })),

  removeConnection: (sourceId, destId) =>
    set((s) => ({
      connections: s.connections.filter((c) => !(c.sourceId === sourceId && c.destId === destId)),
    })),

  syncGraph: (blocks, connections) => set({ blocks, connections }),

  setClipboardBlockType: (type) => set({ clipboardBlockType: type }),
  selectBlock: (blockId) => set({ selectedBlockId: blockId }),
  setDraggingConnection: (state) => set({ draggingConnection: state }),
}));
