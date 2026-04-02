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

interface StellarrState {
  loading: boolean;
  loadingStatus: string;
  loadingProgress: number;
  connected: boolean;
  showSettings: boolean;
  blocks: GridBlock[];
  connections: Connection[];
  grid: GridSettings;
  scanDirectories: ScanDirectory[];
  availablePlugins: PluginInfo[];

  setLoading: (loading: boolean) => void;
  setLoadingStatus: (status: string, progress: number) => void;
  setConnected: (value: boolean) => void;
  setShowSettings: (value: boolean) => void;
  setBlockTestTone: (blockId: string, enabled: boolean) => void;
  setGridSize: (columns: number, rows: number) => void;
  setScanDirectories: (dirs: ScanDirectory[]) => void;
  setAvailablePlugins: (plugins: PluginInfo[]) => void;
  setBlockPlugin: (blockId: string, pluginId: string, pluginName: string) => void;

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
  showSettings: false,
  blocks: [],
  connections: [],
  grid: { columns: 12, rows: 6 },
  scanDirectories: [],
  availablePlugins: [],
  selectedBlockId: null,
  draggingConnection: null,

  setLoading: (loading) => set({ loading }),
  setLoadingStatus: (status, progress) =>
    set({ loadingStatus: status, loadingProgress: progress }),
  setConnected: (value) => set({ connected: value }),
  setShowSettings: (value) => set({ showSettings: value }),

  setBlockTestTone: (blockId, enabled) =>
    set((s) => ({
      blocks: s.blocks.map((b) =>
        b.id === blockId ? { ...b, testTone: enabled } : b,
      ),
    })),

  setGridSize: (columns, rows) => set({ grid: { columns, rows } }),
  setScanDirectories: (dirs) => set({ scanDirectories: dirs }),
  setAvailablePlugins: (plugins) => set({ availablePlugins: plugins }),

  setBlockPlugin: (blockId, pluginId, pluginName) =>
    set((s) => ({
      blocks: s.blocks.map((b) =>
        b.id === blockId ? { ...b, pluginId, pluginName } : b,
      ),
    })),

  addBlock: (block) =>
    set((s) => ({ blocks: [...s.blocks, block] })),

  removeBlock: (blockId) =>
    set((s) => ({
      blocks: s.blocks.filter((b) => b.id !== blockId),
      connections: s.connections.filter(
        (c) => c.sourceId !== blockId && c.destId !== blockId,
      ),
      selectedBlockId:
        s.selectedBlockId === blockId ? null : s.selectedBlockId,
    })),

  moveBlock: (blockId, col, row) =>
    set((s) => ({
      blocks: s.blocks.map((b) =>
        b.id === blockId ? { ...b, col, row } : b,
      ),
    })),

  addConnection: (conn) =>
    set((s) => ({ connections: [...s.connections, conn] })),

  removeConnection: (sourceId, destId) =>
    set((s) => ({
      connections: s.connections.filter(
        (c) => !(c.sourceId === sourceId && c.destId === destId),
      ),
    })),

  syncGraph: (blocks, connections) => set({ blocks, connections }),

  selectBlock: (blockId) => set({ selectedBlockId: blockId }),
  setDraggingConnection: (state) => set({ draggingConnection: state }),
}));
