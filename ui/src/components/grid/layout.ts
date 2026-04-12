export const CELL_SIZE = 72;
export const GAP = 16;
export const STEP = CELL_SIZE + GAP;

export function cellLeft(col: number): number {
  return col * STEP;
}

export function cellTop(row: number): number {
  return row * STEP;
}

export function outputPortX(col: number): number {
  return col * STEP + CELL_SIZE;
}

export function inputPortX(col: number): number {
  return col * STEP;
}

export function gridWidth(columns: number): number {
  return columns * STEP - GAP;
}

export function gridHeight(rows: number): number {
  return rows * STEP - GAP;
}

/**
 * Calculate the Y position for the nth connection on a block edge.
 * Connections are spaced equidistantly from the vertical centre.
 *
 * @param row - block grid row
 * @param count - total connections on this edge
 * @param index - this connection's index (0-based, sorted top to bottom)
 */
export function connectionY(row: number, count: number, index: number): number {
  const centreY = row * STEP + CELL_SIZE / 2;
  if (count <= 1) return centreY;
  const gap = Math.min(16, (CELL_SIZE - 8) / count);
  return centreY + (index - (count - 1) / 2) * gap;
}
