import { useCallback, useEffect, useRef, useState } from 'react';
import { useStore } from '../../store';
import {
  requestAddBlock,
  requestAddConnection,
  requestMoveBlock,
  requestToggleBlockBypass,
} from '../../bridge';
import { GridBlockComponent } from './GridBlock';
import { ConnectionLayer } from './ConnectionLayer';
import { BlockMenu } from './BlockMenu';
import { CELL_SIZE, STEP, cellLeft, cellTop, gridWidth, gridHeight } from './layout';
import styles from './Grid.module.css';

export function Grid() {
  const grid = useStore((s) => s.grid);
  const blocks = useStore((s) => s.blocks);
  const draggingConnection = useStore((s) => s.draggingConnection);
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);
  const gridRef = useRef<HTMLDivElement>(null);

  const selectedBlockId = useStore((s) => s.selectedBlockId);

  const [hoveredCell, setHoveredCell] = useState<{ col: number; row: number } | null>(null);

  // Space key toggles bypass on the selected block
  useEffect(() => {
    const handler = (e: KeyboardEvent) => {
      if (e.code === 'Space' && selectedBlockId) {
        e.preventDefault();
        const block = useStore.getState().blocks.find((b) => b.id === selectedBlockId);
        if (block && block.type !== 'input' && block.type !== 'output') {
          useStore.getState().setBlockBypassed(selectedBlockId, !block.bypassed);
          requestToggleBlockBypass(selectedBlockId);
        }
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [selectedBlockId]);
  const [menuCell, setMenuCell] = useState<{ col: number; row: number } | null>(null);

  const occupiedSet = new Set(blocks.map((b) => `${b.col},${b.row}`));

  const cellFromPoint = useCallback(
    (clientX: number, clientY: number): { col: number; row: number } | null => {
      if (!gridRef.current) return null;
      const rect = gridRef.current.getBoundingClientRect();
      const x = clientX - rect.left;
      const y = clientY - rect.top;
      const col = Math.floor(x / STEP);
      const row = Math.floor(y / STEP);
      if (col < 0 || col >= grid.columns || row < 0 || row >= grid.rows) return null;
      const cellX = x - col * STEP;
      const cellY = y - row * STEP;
      if (cellX > CELL_SIZE || cellY > CELL_SIZE) return null;
      return { col, row };
    },
    [grid],
  );

  const handleDragOver = useCallback((e: React.DragEvent) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
  }, []);

  const handleDrop = useCallback(
    (e: React.DragEvent) => {
      e.preventDefault();
      const cell = cellFromPoint(e.clientX, e.clientY);
      if (!cell) return;

      const moveBlockId = e.dataTransfer.getData('moveBlockId');
      if (!moveBlockId) return;

      const moving = blocks.find((b) => b.id === moveBlockId);
      if (!moving) return;

      const occupied = occupiedSet.has(`${cell.col},${cell.row}`);
      if (occupied && !(moving.col === cell.col && moving.row === cell.row)) return;
      if (moving.col !== cell.col || moving.row !== cell.row)
        requestMoveBlock(moveBlockId, cell.col, cell.row);
    },
    [blocks, cellFromPoint, occupiedSet],
  );

  const handleMouseMove = useCallback(
    (e: React.MouseEvent) => {
      if (draggingConnection && gridRef.current) {
        const rect = gridRef.current.getBoundingClientRect();
        setDraggingConnection({
          ...draggingConnection,
          mouseX: e.clientX - rect.left,
          mouseY: e.clientY - rect.top,
        });
      }
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
        if (draggingConnection.portType === 'output' && dropPortType === 'input')
          requestAddConnection(draggingConnection.blockId, dropBlockId);
        else if (draggingConnection.portType === 'input' && dropPortType === 'output')
          requestAddConnection(dropBlockId, draggingConnection.blockId);
      }

      setDraggingConnection(null);
    },
    [draggingConnection, setDraggingConnection],
  );

  const handleCellClick = useCallback(
    (col: number, row: number) => {
      if (occupiedSet.has(`${col},${row}`)) return;
      setMenuCell({ col, row });
    },
    [occupiedSet],
  );

  const handleMenuSelect = useCallback(
    (type: string) => {
      if (!menuCell) return;

      // Check if a connection passes through this cell (same row, between source and dest)
      const blockMap = new Map(blocks.map((b) => [b.id, b]));
      let spliceSourceId: string | undefined;
      let spliceDestId: string | undefined;

      for (const conn of useStore.getState().connections) {
        const src = blockMap.get(conn.sourceId);
        const dst = blockMap.get(conn.destId);
        if (!src || !dst) continue;
        if (src.row !== menuCell.row || dst.row !== menuCell.row) continue;

        const minCol = Math.min(src.col, dst.col);
        const maxCol = Math.max(src.col, dst.col);

        if (menuCell.col > minCol && menuCell.col < maxCol) {
          spliceSourceId = conn.sourceId;
          spliceDestId = conn.destId;
          break;
        }
      }

      requestAddBlock(type, menuCell.col, menuCell.row, spliceSourceId, spliceDestId);
      setMenuCell(null);
    },
    [menuCell, blocks],
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
      onMouseLeave={() => setHoveredCell(null)}
      className={styles.grid}
      style={{ width: gw, height: gh }}
    >
      {/* Cell backgrounds */}
      {Array.from({ length: grid.rows }, (_, row) =>
        Array.from({ length: grid.columns }, (_, col) => {
          const key = `${col},${row}`;
          const occupied = occupiedSet.has(key);
          const isHovered = !occupied && hoveredCell?.col === col && hoveredCell?.row === row;
          const isMenuOpen = menuCell?.col === col && menuCell?.row === row;

          const cellClassName = `${styles.cell} ${occupied ? styles.cellOccupied : ''} ${isMenuOpen ? styles.cellActive : ''}`;

          const cellContent = (
            <div
              key={key}
              onMouseEnter={() => {
                if (!occupied) setHoveredCell({ col, row });
              }}
              onMouseLeave={() => {
                if (hoveredCell?.col === col && hoveredCell?.row === row) setHoveredCell(null);
              }}
              onClick={() => handleCellClick(col, row)}
              className={cellClassName}
              style={{
                left: cellLeft(col),
                top: cellTop(row),
                width: CELL_SIZE,
                height: CELL_SIZE,
              }}
            >
              {(isHovered || isMenuOpen) && <span className={styles.cellPlus}>+</span>}
            </div>
          );

          if (isMenuOpen) {
            return (
              <BlockMenu
                key={key}
                open={true}
                onSelect={handleMenuSelect}
                onClose={() => setMenuCell(null)}
              >
                {cellContent}
              </BlockMenu>
            );
          }

          return cellContent;
        }),
      )}

      <ConnectionLayer gridRef={gridRef} />

      {blocks.map((block) => (
        <GridBlockComponent key={block.id} block={block} />
      ))}
    </div>
  );
}
