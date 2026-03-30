import { useCallback, useRef } from 'react';
import { useStore } from '../store';
import { requestAddBlock, requestAddConnection } from '../bridge';
import { GridBlockComponent } from './GridBlock';
import { ConnectionLayer } from './ConnectionLayer';
import { colors } from './colors';

const CELL_SIZE = 80;

export function Grid() {
  const grid = useStore((s) => s.grid);
  const blocks = useStore((s) => s.blocks);
  const draggingConnection = useStore((s) => s.draggingConnection);
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);
  const gridRef = useRef<HTMLDivElement>(null);

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'copy';
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      const blockType = e.dataTransfer.getData('blockType');
      if (!blockType || !gridRef.current) return;

      const rect = gridRef.current.getBoundingClientRect();
      const col = Math.floor((e.clientX - rect.left) / CELL_SIZE);
      const row = Math.floor((e.clientY - rect.top) / CELL_SIZE);

      if (col < 0 || col >= grid.columns || row < 0 || row >= grid.rows)
        return;

      // Check cell is empty
      const occupied = blocks.some((b) => b.col === col && b.row === row);
      if (occupied) return;

      requestAddBlock(blockType, col, row);
    },
    [grid, blocks],
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

      // Check if we dropped on an input port
      const target = e.target as HTMLElement;
      const portBlockId = target.getAttribute('data-port-block-id');
      const portType = target.getAttribute('data-port-type');

      if (portBlockId && portType === 'input') {
        requestAddConnection(draggingConnection.sourceId, portBlockId);
      }

      setDraggingConnection(null);
    },
    [draggingConnection, setDraggingConnection],
  );

  const gridWidth = grid.columns * CELL_SIZE;
  const gridHeight = grid.rows * CELL_SIZE;

  return (
    <div
      ref={gridRef}
      onDragOver={handleDragOver}
      onDrop={handleDrop}
      onMouseMove={handleMouseMove}
      onMouseUp={handleMouseUp}
      style={{
        position: 'relative',
        width: gridWidth,
        height: gridHeight,
        background: colors.bg,
        overflow: 'hidden',
      }}
    >
      {/* Grid lines */}
      <svg
        style={{
          position: 'absolute',
          top: 0,
          left: 0,
          width: gridWidth,
          height: gridHeight,
          pointerEvents: 'none',
        }}
      >
        {Array.from({ length: grid.columns + 1 }, (_, i) => (
          <line
            key={`v${i}`}
            x1={i * CELL_SIZE}
            y1={0}
            x2={i * CELL_SIZE}
            y2={gridHeight}
            stroke={colors.border}
            strokeWidth={0.5}
          />
        ))}
        {Array.from({ length: grid.rows + 1 }, (_, i) => (
          <line
            key={`h${i}`}
            x1={0}
            y1={i * CELL_SIZE}
            x2={gridWidth}
            y2={i * CELL_SIZE}
            stroke={colors.border}
            strokeWidth={0.5}
          />
        ))}
      </svg>

      <ConnectionLayer cellSize={CELL_SIZE} />

      {blocks.map((block) => (
        <GridBlockComponent key={block.id} block={block} cellSize={CELL_SIZE} />
      ))}
    </div>
  );
}
