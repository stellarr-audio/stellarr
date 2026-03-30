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

export function portY(row: number): number {
  return row * STEP + CELL_SIZE / 2;
}

export function gridWidth(columns: number): number {
  return columns * STEP - GAP;
}

export function gridHeight(rows: number): number {
  return rows * STEP - GAP;
}
