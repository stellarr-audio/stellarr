import { useMemo, useState } from 'react';
import { IoCloseSharp, IoAddSharp, IoLockClosedOutline } from 'react-icons/io5';
import { useStore } from '../../store';
import { requestMoveBlock, requestSetGridSize } from '../../bridge';
import { CELL_SIZE, GAP, STEP, gridWidth, gridHeight } from './layout';
import styles from './GridResizer.module.css';

const MIN_COLS = 1;
const MIN_ROWS = 1;
const MAX_COLS = 20;
const MAX_ROWS = 12;

type HoverPreview =
  | { kind: 'delete-col'; index: number }
  | { kind: 'delete-row'; index: number }
  | { kind: 'add-col' }
  | { kind: 'add-row' }
  | null;

interface Props {
  children: React.ReactNode;
}

/**
 * Wraps the Grid with a left + top gutter of per-row / per-col delete chips
 * and trailing add pills. Only empty rows / columns can be deleted (rule
 * surfaced via a muted lock icon). Hover a delete chip to see a dashed
 * preview line through the row/col that would be removed.
 */
export function GridResizer({ children }: Props) {
  const grid = useStore((s) => s.grid);
  const blocks = useStore((s) => s.blocks);
  const setGridSize = useStore((s) => s.setGridSize);
  const moveBlock = useStore((s) => s.moveBlock);

  const [hover, setHover] = useState<HoverPreview>(null);

  const occupiedCols = useMemo(() => {
    const s = new Set<number>();
    for (const b of blocks) s.add(b.col);
    return s;
  }, [blocks]);

  const occupiedRows = useMemo(() => {
    const s = new Set<number>();
    for (const b of blocks) s.add(b.row);
    return s;
  }, [blocks]);

  const gw = gridWidth(grid.columns);
  const gh = gridHeight(grid.rows);

  const canAddCol = grid.columns < MAX_COLS;
  const canAddRow = grid.rows < MAX_ROWS;

  const deleteCol = (col: number) => {
    if (occupiedCols.has(col)) return;
    if (grid.columns <= MIN_COLS) return;
    // Shift blocks in columns > col one step left.
    for (const b of blocks) {
      if (b.col > col) {
        moveBlock(b.id, b.col - 1, b.row);
        requestMoveBlock(b.id, b.col - 1, b.row);
      }
    }
    const nextCols = grid.columns - 1;
    setGridSize(nextCols, grid.rows);
    requestSetGridSize(nextCols, grid.rows);
  };

  const deleteRow = (row: number) => {
    if (occupiedRows.has(row)) return;
    if (grid.rows <= MIN_ROWS) return;
    for (const b of blocks) {
      if (b.row > row) {
        moveBlock(b.id, b.col, b.row - 1);
        requestMoveBlock(b.id, b.col, b.row - 1);
      }
    }
    const nextRows = grid.rows - 1;
    setGridSize(grid.columns, nextRows);
    requestSetGridSize(grid.columns, nextRows);
  };

  const addCol = () => {
    if (!canAddCol) return;
    const nextCols = grid.columns + 1;
    setGridSize(nextCols, grid.rows);
    requestSetGridSize(nextCols, grid.rows);
  };

  const addRow = () => {
    if (!canAddRow) return;
    const nextRows = grid.rows + 1;
    setGridSize(grid.columns, nextRows);
    requestSetGridSize(grid.columns, nextRows);
  };

  const pillSize = 20;
  const addGap = 8;
  const gutterSize = pillSize + addGap;

  // Hover preview — 1px dashed line through the centre of the target row
  // or column, in the Grid's local coordinate space.
  let previewStyle: React.CSSProperties | null = null;
  let previewClass: string | null = null;
  if (hover?.kind === 'delete-col') {
    const x = hover.index * STEP + CELL_SIZE / 2;
    previewStyle = { left: x, top: -8, bottom: -8, width: 0 };
    previewClass = styles.previewColDanger;
  } else if (hover?.kind === 'delete-row') {
    const y = hover.index * STEP + CELL_SIZE / 2;
    previewStyle = { top: y, left: -8, right: -8, height: 0 };
    previewClass = styles.previewRowDanger;
  } else if (hover?.kind === 'add-col') {
    // Line aligns with the add-col chip's horizontal centre.
    const x = grid.columns * STEP - GAP + addGap + pillSize / 2;
    previewStyle = { left: x, top: -8, bottom: -8, width: 0 };
    previewClass = styles.previewColAdd;
  } else if (hover?.kind === 'add-row') {
    const y = grid.rows * STEP - GAP + addGap + pillSize / 2;
    previewStyle = { top: y, left: -8, right: -8, height: 0 };
    previewClass = styles.previewRowAdd;
  }

  // Each chip is absolutely positioned at the centre of its target cell so
  // alignment is exact regardless of flex or font-glyph metrics.
  const colChipTop = 0;
  const rowChipLeft = 0;
  const gridOriginX = gutterSize;
  const gridOriginY = gutterSize;
  const addColX = gridOriginX + grid.columns * STEP - GAP + addGap;
  const addRowY = gridOriginY + grid.rows * STEP - GAP + addGap;

  return (
    <div
      className={styles.frame}
      style={{
        width: gridOriginX + gw + pillSize + addGap,
        height: gridOriginY + gh + pillSize + addGap,
      }}
    >
      {/* Column chips — one per column, centred over its cell */}
      {Array.from({ length: grid.columns }, (_, c) => {
        const locked = occupiedCols.has(c);
        const disabled = locked || grid.columns <= MIN_COLS;
        const centreX = gridOriginX + c * STEP + CELL_SIZE / 2;
        return (
          <div
            key={`col-${c}`}
            className={
              locked ? styles.chipLocked : disabled ? styles.chipDisabled : styles.chipDelete
            }
            style={{
              position: 'absolute',
              left: centreX - pillSize / 2,
              top: colChipTop,
              width: pillSize,
              height: pillSize,
            }}
            onMouseEnter={() => {
              if (!disabled) setHover({ kind: 'delete-col', index: c });
            }}
            onMouseLeave={() => setHover(null)}
            onClick={() => deleteCol(c)}
            title={
              locked
                ? 'Column contains blocks — cannot delete'
                : grid.columns <= MIN_COLS
                  ? 'Minimum is 1 column'
                  : `Delete column ${c + 1}`
            }
          >
            {locked ? <IoLockClosedOutline size={13} /> : <IoCloseSharp size={13} />}
          </div>
        );
      })}

      {/* Trailing add-column */}
      <div
        className={canAddCol ? styles.chipAdd : styles.chipDisabled}
        style={{
          position: 'absolute',
          left: addColX,
          top: colChipTop,
          width: pillSize,
          height: pillSize,
        }}
        onMouseEnter={() => {
          if (canAddCol) setHover({ kind: 'add-col' });
        }}
        onMouseLeave={() => setHover(null)}
        onClick={addCol}
        title={canAddCol ? 'Add column' : `Maximum is ${MAX_COLS} columns`}
      >
        <IoAddSharp size={13} />
      </div>

      {/* Row chips — one per row, centred on its cell */}
      {Array.from({ length: grid.rows }, (_, r) => {
        const locked = occupiedRows.has(r);
        const disabled = locked || grid.rows <= MIN_ROWS;
        const centreY = gridOriginY + r * STEP + CELL_SIZE / 2;
        return (
          <div
            key={`row-${r}`}
            className={
              locked ? styles.chipLocked : disabled ? styles.chipDisabled : styles.chipDelete
            }
            style={{
              position: 'absolute',
              left: rowChipLeft,
              top: centreY - pillSize / 2,
              width: pillSize,
              height: pillSize,
            }}
            onMouseEnter={() => {
              if (!disabled) setHover({ kind: 'delete-row', index: r });
            }}
            onMouseLeave={() => setHover(null)}
            onClick={() => deleteRow(r)}
            title={
              locked
                ? 'Row contains blocks — cannot delete'
                : grid.rows <= MIN_ROWS
                  ? 'Minimum is 1 row'
                  : `Delete row ${r + 1}`
            }
          >
            {locked ? <IoLockClosedOutline size={13} /> : <IoCloseSharp size={13} />}
          </div>
        );
      })}

      {/* Trailing add-row */}
      <div
        className={canAddRow ? styles.chipAdd : styles.chipDisabled}
        style={{
          position: 'absolute',
          left: rowChipLeft,
          top: addRowY,
          width: pillSize,
          height: pillSize,
        }}
        onMouseEnter={() => {
          if (canAddRow) setHover({ kind: 'add-row' });
        }}
        onMouseLeave={() => setHover(null)}
        onClick={addRow}
        title={canAddRow ? 'Add row' : `Maximum is ${MAX_ROWS} rows`}
      >
        <IoAddSharp size={13} />
      </div>

      {/* Grid surface */}
      <div
        className={styles.gridSurface}
        style={{
          position: 'absolute',
          left: gridOriginX,
          top: gridOriginY,
          width: gw,
          height: gh,
        }}
      >
        {children}
        {previewStyle && previewClass && <div className={previewClass} style={previewStyle} />}
      </div>
    </div>
  );
}
