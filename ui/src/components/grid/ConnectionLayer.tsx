import type { RefObject } from 'react';
import { useStore } from '../../store';
import { colors } from '../common/colors';
import { CELL_SIZE, GAP, outputPortX, inputPortX, portY, gridWidth, gridHeight } from './layout';

const wireColors = [
  '#00b4ff',
  '#ff2d7b',
  '#00ff9d',
  '#ffaa00',
  '#a855f7',
  '#00ffe0',
  '#ff6b6b',
  '#6bff6b',
];

interface Props {
  gridRef: RefObject<HTMLDivElement | null>;
}

function orthogonalPath(x1: number, y1: number, x2: number, y2: number): string {
  // Route through the gap space:
  // 1. Exit source going right into the gap
  // 2. Travel vertically to destination row
  // 3. Enter destination going right from the gap
  const midX = x1 + GAP / 2;

  if (Math.abs(y1 - y2) < 1) {
    // Same row — straight horizontal line
    return `M ${x1} ${y1} L ${x2} ${y2}`;
  }

  if (x2 > x1) {
    // Destination is to the right — simple L-shape through gap
    const bendX = x1 + GAP / 2;
    return `M ${x1} ${y1} L ${bendX} ${y1} L ${bendX} ${y2} L ${x2} ${y2}`;
  }

  // Destination is to the left or same column — route around via gap below/above
  const offsetY = y2 > y1 ? CELL_SIZE / 2 + GAP / 2 : -(CELL_SIZE / 2 + GAP / 2);
  return [
    `M ${x1} ${y1}`,
    `L ${midX} ${y1}`,
    `L ${midX} ${y1 + offsetY}`,
    `L ${x2 - GAP / 2} ${y1 + offsetY}`,
    `L ${x2 - GAP / 2} ${y2}`,
    `L ${x2} ${y2}`,
  ].join(' ');
}

export function ConnectionLayer(_props: Props) {
  const blocks = useStore((s) => s.blocks);
  const connections = useStore((s) => s.connections);
  const grid = useStore((s) => s.grid);
  const dragging = useStore((s) => s.draggingConnection);

  const blockMap = new Map(blocks.map((b) => [b.id, b]));

  const gw = gridWidth(grid.columns);
  const gh = gridHeight(grid.rows);

  return (
    <svg
      style={{
        position: 'absolute',
        top: 0,
        left: 0,
        width: gw,
        height: gh,
        pointerEvents: 'none',
        zIndex: 1,
      }}
    >
      {connections.map((conn, i) => {
        const src = blockMap.get(conn.sourceId);
        const dst = blockMap.get(conn.destId);
        if (!src || !dst) return null;

        const x1 = outputPortX(src.col);
        const y1 = portY(src.row);
        const x2 = inputPortX(dst.col);
        const y2 = portY(dst.row);

        const wireColor = wireColors[i % wireColors.length];

        return (
          <path
            key={i}
            d={orthogonalPath(x1, y1, x2, y2)}
            stroke={wireColor + 'bb'}
            strokeWidth={2}
            fill="none"
          />
        );
      })}

      {dragging &&
        (() => {
          const blk = blockMap.get(dragging.blockId);
          if (!blk) return null;
          const x1 = dragging.portType === 'output' ? outputPortX(blk.col) : inputPortX(blk.col);
          const y1 = portY(blk.row);
          return (
            <line
              x1={x1}
              y1={y1}
              x2={dragging.mouseX}
              y2={dragging.mouseY}
              stroke={colors.connectionLine}
              strokeWidth={2}
              strokeDasharray="4 4"
            />
          );
        })()}
    </svg>
  );
}
