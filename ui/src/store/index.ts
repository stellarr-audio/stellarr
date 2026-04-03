import { create } from 'zustand';

export interface GridBlock {
  id: string;
  type: string;
  name: string;
  col: number;
  row: number;
  nodeId: number;
  testTone?: boolean;
  pluginId?: string;
  pluginName?: string;
  pluginFormat?: string;
  displayName?: string;
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
  cpuPercent: number;
  memoryMB: number;
  justSaved: boolean;
  totalMemoryMB: number;
  scenes: Scene[];
  activeSceneIndex: number;
  midiMappings: MidiMapping[];
  midiLearning: boolean;

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
  setJustSaved: (value: boolean) => void;
  setScenes: (scenes: Scene[], activeSceneIndex: number) => void;
  setMidiMappings: (mappings: MidiMapping[], learning: boolean) => void;

  addBlock: (block: GridBlock) => void;
  removeBlock: (blockId: string) => void;
  moveBlock: (blockId: string, col: number, row: number) => void;
  addConnection: (conn: Connection) => void;
  removeConnection: (sourceId: string, destId: string) => void;
  syncGraph: (blocks: GridBlock[], connections: Connection[]) => void;

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
  cpuPercent: 0,
  memoryMB: 0,
  justSaved: false,
  totalMemoryMB: 1,
  scenes: [],
  activeSceneIndex: -1,
  midiMappings: [],
  midiLearning: false,
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

  setJustSaved: (value) => set({ justSaved: value }),

  setScenes: (scenes, activeSceneIndex) => set({ scenes, activeSceneIndex }),

  setMidiMappings: (mappings, learning) => set({ midiMappings: mappings, midiLearning: learning }),

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

  selectBlock: (blockId) => set({ selectedBlockId: blockId }),
  setDraggingConnection: (state) => set({ draggingConnection: state }),
}));
