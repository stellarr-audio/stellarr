import { create } from 'zustand';

export interface GridBlock {
  id: string;
  type: string;
  name: string;
  col: number;
  row: number;
  nodeId: number;
  testTone?: boolean;
}

export interface Connection {
  sourceId: string;
  destId: string;
}

export interface GridSettings {
  columns: number;
  rows: number;
}

interface StellarrState {
  connected: boolean;
  blocks: GridBlock[];
  connections: Connection[];
  grid: GridSettings;

  setConnected: (value: boolean) => void;
  setBlockTestTone: (blockId: string, enabled: boolean) => void;
  setGridSize: (columns: number, rows: number) => void;

  // Store mutations — called only by bridge when C++ confirms
  addBlock: (block: GridBlock) => void;
  removeBlock: (blockId: string) => void;
  moveBlock: (blockId: string, col: number, row: number) => void;
  addConnection: (conn: Connection) => void;
  removeConnection: (sourceId: string, destId: string) => void;
  syncGraph: (blocks: GridBlock[], connections: Connection[]) => void;

  // UI-only state
  selectedBlockId: string | null;
  selectBlock: (blockId: string | null) => void;

  draggingConnection: { blockId: string; portType: 'input' | 'output'; mouseX: number; mouseY: number } | null;
  setDraggingConnection: (
    state: { blockId: string; portType: 'input' | 'output'; mouseX: number; mouseY: number } | null,
  ) => void;
}

export const useStore = create<StellarrState>((set) => ({
  connected: false,
  blocks: [],
  connections: [],
  grid: { columns: 12, rows: 6 },
  selectedBlockId: null,
  draggingConnection: null,

  setConnected: (value) => set({ connected: value }),

  setBlockTestTone: (blockId, enabled) =>
    set((s) => ({
      blocks: s.blocks.map((b) =>
        b.id === blockId ? { ...b, testTone: enabled } : b,
      ),
    })),

  setGridSize: (columns, rows) => set({ grid: { columns, rows } }),

  addBlock: (block) =>
    set((s) => ({ blocks: [...s.blocks, block] })),

  removeBlock: (blockId) =>
    set((s) => ({
      blocks: s.blocks.filter((b) => b.id !== blockId),
      connections: s.connections.filter(
        (c) => c.sourceId !== blockId && c.destId !== blockId,
      ),
      selectedBlockId: s.selectedBlockId === blockId ? null : s.selectedBlockId,
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
