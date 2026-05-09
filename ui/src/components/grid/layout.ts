import { useStore } from '../../store';

export const ZOOM_PRESETS = {
  S: { cellSize: 72, gap: 16 },
  M: { cellSize: 88, gap: 24 },
  L: { cellSize: 104, gap: 32 },
} as const;

export type CellZoomLevel = keyof typeof ZOOM_PRESETS;

export interface GridLayout {
  cellSize: number;
  gap: number;
  // Distance between successive cells along either axis (cellSize + gap).
  step: number;
  cellLeft: (col: number) => number;
  cellTop: (row: number) => number;
  // gridWidth / gridHeight: total pixel span occupied by N cells in a row /
  // column with N-1 gaps between them. Equivalent to (n * step - gap) for
  // n >= 1; the Math.max guard keeps the zero-cells case safe (returns 0).
  gridWidth: (cols: number) => number;
  gridHeight: (rows: number) => number;
  outputPortX: (col: number) => number;
  inputPortX: (col: number) => number;
  connectionY: (row: number, count: number, index: number) => number;
}

export function useGridLayout(): GridLayout {
  const zoom = useStore((s) => s.cellZoom);
  const { cellSize, gap } = ZOOM_PRESETS[zoom];
  const step = cellSize + gap;
  // Plain object literal; closure functions recreated per render. Acceptable
  // at grid render rates — useMemo would save allocations on non-zoom renders
  // but complicate the hook for minimal gain.
  return {
    cellSize,
    gap,
    step,
    cellLeft: (col) => col * step,
    cellTop: (row) => row * step,
    gridWidth: (cols) => cols * cellSize + Math.max(0, cols - 1) * gap,
    gridHeight: (rows) => rows * cellSize + Math.max(0, rows - 1) * gap,
    outputPortX: (col) => col * step + cellSize,
    inputPortX: (col) => col * step,
    connectionY: (row, count, index) => {
      const centreY = row * step + cellSize / 2;
      if (count <= 1) return centreY;
      const connGap = Math.min(16, (cellSize - 8) / count);
      return centreY + (index - (count - 1) / 2) * connGap;
    },
  };
}
