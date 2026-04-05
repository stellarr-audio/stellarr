import type { RefObject } from 'react';
import { useStore } from '../../store';
import { colors } from '../common/colors';
import { CELL_SIZE, GAP, outputPortX, inputPortX, portY, gridWidth, gridHeight } from './layout';
import styles from './ConnectionLayer.module.css';

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

  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const blockMap = new Map(blocks.map((b) => [b.id, b]));

  // Find all connections on live routes (input→output) through the selected block.
  // A block with bypassed=true + bypassMode="mute" is a dead end — signal stops there.
  const liveConnections = new Set<number>();
  if (selectedBlockId) {
    const downstream = new Map<string, string[]>();
    const upstream = new Map<string, string[]>();
    for (const c of connections) {
      downstream.set(c.sourceId, [...(downstream.get(c.sourceId) ?? []), c.destId]);
      upstream.set(c.destId, [...(upstream.get(c.destId) ?? []), c.sourceId]);
    }

    const isMuted = (id: string) => {
      const b = blockMap.get(id);
      return b?.bypassed && b?.bypassMode === 'mute';
    };

    // Walk downstream from selected block, collecting all paths that reach an output.
    // Returns the set of block IDs on live (output-reaching) paths.
    const downstreamLive = new Set<string>();
    const canReachOutput = (id: string, visited: Set<string>): boolean => {
      if (visited.has(id)) return false;
      visited.add(id);
      const blk = blockMap.get(id);
      if (!blk) return false;
      if (blk.type === 'output') {
        downstreamLive.add(id);
        return true;
      }
      if (id !== selectedBlockId && isMuted(id)) return false;

      let reachable = false;
      for (const next of downstream.get(id) ?? []) {
        if (canReachOutput(next, visited)) {
          downstreamLive.add(id);
          reachable = true;
        }
      }
      return reachable;
    };
    canReachOutput(selectedBlockId, new Set());

    // Walk upstream from selected block, collecting all paths that reach an input.
    const upstreamLive = new Set<string>();
    const canReachInput = (id: string, visited: Set<string>): boolean => {
      if (visited.has(id)) return false;
      visited.add(id);
      const blk = blockMap.get(id);
      if (!blk) return false;
      if (blk.type === 'input') {
        upstreamLive.add(id);
        return true;
      }
      if (id !== selectedBlockId && isMuted(id)) return false;

      let reachable = false;
      for (const prev of upstream.get(id) ?? []) {
        if (canReachInput(prev, visited)) {
          upstreamLive.add(id);
          reachable = true;
        }
      }
      return reachable;
    };
    canReachInput(selectedBlockId, new Set());

    // A connection is live if both endpoints are on the combined live route
    const allLive = new Set([...downstreamLive, ...upstreamLive]);
    connections.forEach((conn, i) => {
      if (allLive.has(conn.sourceId) && allLive.has(conn.destId)) liveConnections.add(i);
    });
  }

  const hasSelection = selectedBlockId !== null;

  const gw = gridWidth(grid.columns);
  const gh = gridHeight(grid.rows);

  return (
    <svg className={styles.svg} style={{ width: gw, height: gh }}>
      {connections.map((conn, i) => {
        const src = blockMap.get(conn.sourceId);
        const dst = blockMap.get(conn.destId);
        if (!src || !dst) return null;

        const x1 = outputPortX(src.col);
        const y1 = portY(src.row);
        const x2 = inputPortX(dst.col);
        const y2 = portY(dst.row);

        const wireColor = wireColors[i % wireColors.length];
        const isLive = liveConnections.has(i);
        const stroke = hasSelection
          ? isLive
            ? colors.warning
            : wireColor + '26'
          : wireColor + 'bb';

        return (
          <path
            key={i}
            d={orthogonalPath(x1, y1, x2, y2)}
            stroke={stroke}
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
