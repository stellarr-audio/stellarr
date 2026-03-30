import { useCallback, useRef } from 'react';
import { useStore } from '../store';
import { requestAddBlock, requestAddConnection, requestMoveBlock } from '../bridge';
import { GridBlockComponent } from './GridBlock';
import { ConnectionLayer } from './ConnectionLayer';
import { colors } from './colors';
import { CELL_SIZE, STEP, cellLeft, cellTop, gridWidth, gridHeight } from './layout';

export function Grid() {
  const grid = useStore((s) => s.grid);
  const blocks = useStore((s) => s.blocks);
  const draggingConnection = useStore((s) => s.draggingConnection);
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);
  const gridRef = useRef<HTMLDivElement>(null);

  const cellFromPoint = useCallback(
    (clientX: number, clientY: number): { col: number; row: number } | null => {
      if (!gridRef.current) return null;
      const rect = gridRef.current.getBoundingClientRect();
      const x = clientX - rect.left;
      const y = clientY - rect.top;
      const col = Math.floor(x / STEP);
      const row = Math.floor(y / STEP);
      if (col < 0 || col >= grid.columns || row < 0 || row >= grid.rows) return null;

      // Check the click is within the cell area, not in the gap
      const cellX = x - col * STEP;
      const cellY = y - row * STEP;
      if (cellX > CELL_SIZE || cellY > CELL_SIZE) return null;

      return { col, row };
    },
    [grid],
  );

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      const cell = cellFromPoint(e.clientX, e.clientY);
      if (!cell) return;

      const blockType = e.dataTransfer.getData('blockType');
      const moveBlockId = e.dataTransfer.getData('moveBlockId');

      const occupied = blocks.some(
        (b) => b.col === cell.col && b.row === cell.row,
      );

      if (moveBlockId) {
        const moving = blocks.find((b) => b.id === moveBlockId);
        if (!moving) return;
        // Allow drop on the block's own cell (no-op) or an empty cell
        if (occupied && !(moving.col === cell.col && moving.row === cell.row))
          return;
        if (moving.col !== cell.col || moving.row !== cell.row)
          requestMoveBlock(moveBlockId, cell.col, cell.row);
      } else if (blockType) {
        if (occupied) return;
        requestAddBlock(blockType, cell.col, cell.row);
      }
    },
    [blocks, cellFromPoint],
  );

  const handleMouseMove = useCallback(
    (e: React.MouseEvent) => {
      if (!draggingConnection || !gridRef.current) return;
      const rect = gridRef.current.getBoundingClientRect();
      setDraggingConnection({
        ...draggingConnection,
        mouseX: e.clientX - rect.left,
        mouseY: e.clientY - rect.top,
      });
    },
    [draggingConnection, setDraggingConnection],
  );

  const handleMouseUp = useCallback(
    (e: React.MouseEvent) => {
      if (!draggingConnection) return;

      const target = e.target as HTMLElement;
      const dropBlockId = target.getAttribute('data-port-block-id');
      const dropPortType = target.getAttribute('data-port-type');

      if (dropBlockId && dropPortType) {
        // Resolve direction: output→input
        if (draggingConnection.portType === 'output' && dropPortType === 'input') {
          requestAddConnection(draggingConnection.blockId, dropBlockId);
        } else if (draggingConnection.portType === 'input' && dropPortType === 'output') {
          requestAddConnection(dropBlockId, draggingConnection.blockId);
        }
      }

      setDraggingConnection(null);
    },
    [draggingConnection, setDraggingConnection],
  );

  const gw = gridWidth(grid.columns);
  const gh = gridHeight(grid.rows);

  return (
    <div
      ref={gridRef}
      onDragOver={handleDragOver}
      onDrop={handleDrop}
      onMouseMove={handleMouseMove}
      onMouseUp={handleMouseUp}
      style={{
        position: 'relative',
        width: gw,
        height: gh,
        background: colors.bg,
        overflow: 'hidden',
      }}
    >
      {/* Cell backgrounds */}
      {Array.from({ length: grid.rows }, (_, row) =>
        Array.from({ length: grid.columns }, (_, col) => (
          <div
            key={`cell-${col}-${row}`}
            style={{
              position: 'absolute',
              left: cellLeft(col),
              top: cellTop(row),
              width: CELL_SIZE,
              height: CELL_SIZE,
              background: colors.cell,
              border: `1px solid ${colors.border}`,
              boxSizing: 'border-box',
            }}
          />
        )),
      )}

      <ConnectionLayer gridRef={gridRef} />

      {blocks.map((block) => (
        <GridBlockComponent key={block.id} block={block} />
      ))}
    </div>
  );
}
