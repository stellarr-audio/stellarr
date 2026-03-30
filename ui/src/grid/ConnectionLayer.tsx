import { useStore } from '../store';
import { colors } from './colors';

interface Props {
  cellSize: number;
}

export function ConnectionLayer({ cellSize }: Props) {
  const blocks = useStore((s) => s.blocks);
  const connections = useStore((s) => s.connections);
  const grid = useStore((s) => s.grid);
  const dragging = useStore((s) => s.draggingConnection);

  const blockMap = new Map(blocks.map((b) => [b.id, b]));

  const portX = (col: number, isOutput: boolean) =>
    col * cellSize + (isOutput ? cellSize - 2 : 0);

  const portY = (row: number) => row * cellSize + (cellSize - 2) / 2;

  return (
    <svg
      style={{
        position: 'absolute',
        top: 0,
        left: 0,
        width: grid.columns * cellSize,
        height: grid.rows * cellSize,
        pointerEvents: 'none',
        zIndex: 1,
      }}
    >
      {connections.map((conn, i) => {
        const src = blockMap.get(conn.sourceId);
        const dst = blockMap.get(conn.destId);
        if (!src || !dst) return null;

        const x1 = portX(src.col, true);
        const y1 = portY(src.row);
        const x2 = portX(dst.col, false);
        const y2 = portY(dst.row);
        const cx = (x1 + x2) / 2;

        return (
          <path
            key={i}
            d={`M ${x1} ${y1} C ${cx} ${y1}, ${cx} ${y2}, ${x2} ${y2}`}
            stroke={colors.connectionLine}
            strokeWidth={2}
            fill="none"
          />
        );
      })}

      {dragging && (() => {
        const src = blockMap.get(dragging.sourceId);
        if (!src) return null;
        const x1 = portX(src.col, true);
        const y1 = portY(src.row);
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
