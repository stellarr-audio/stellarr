import { useMemo, useState } from 'react';
import { useStore } from '../../store';
import { colors } from '../common/colors';
import {
  CELL_SIZE,
  GAP,
  outputPortX,
  inputPortX,
  connectionY,
  gridWidth,
  gridHeight,
} from './layout';
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
  onConnectionClick?: (e: React.MouseEvent, sourceId: string, destId: string) => void;
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

export function ConnectionLayer({ onConnectionClick }: Props) {
  const blocks = useStore((s) => s.blocks);
  const connections = useStore((s) => s.connections);
  const grid = useStore((s) => s.grid);
  const dragging = useStore((s) => s.draggingConnection);

  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const [hoveredConn, setHoveredConn] = useState<number | null>(null);

  // Find all connections on live routes (input→output) through the selected block.
  // Memoised so the DFS only re-runs when blocks, connections, or selection change.
  const liveConnections = useMemo(() => {
    const live = new Set<number>();
    if (!selectedBlockId) return live;

    const bMap = new Map(blocks.map((b) => [b.id, b]));
    const downstream = new Map<string, string[]>();
    const upstream = new Map<string, string[]>();
    for (const c of connections) {
      downstream.set(c.sourceId, [...(downstream.get(c.sourceId) ?? []), c.destId]);
      upstream.set(c.destId, [...(upstream.get(c.destId) ?? []), c.sourceId]);
    }

    // A block with bypass mode "mute" silences both input and output,
    // effectively cutting the signal chain — treat as a dead end in route tracing.
    const isMuted = (id: string) => {
      const b = bMap.get(id);
      return b?.bypassed && b?.bypassMode === 'mute';
    };

    // Memoised DFS: walk a direction collecting blocks that reach a target type.
    // Uses `memo` to cache results so diamond/convergent paths all get counted.
    // `path` tracks the current recursion stack for cycle detection only.
    const walkReachable = (
      startId: string,
      adj: Map<string, string[]>,
      targetType: string,
    ): Set<string> => {
      const reachable = new Set<string>();
      const memo = new Map<string, boolean>();

      const canReach = (id: string, path: Set<string>): boolean => {
        if (memo.has(id)) return memo.get(id)!;
        if (path.has(id)) return false;
        path.add(id);

        const blk = bMap.get(id);
        if (!blk) {
          memo.set(id, false);
          path.delete(id);
          return false;
        }
        if (blk.type === targetType) {
          reachable.add(id);
          memo.set(id, true);
          path.delete(id);
          return true;
        }
        if (isMuted(id)) {
          memo.set(id, false);
          path.delete(id);
          return false;
        }

        let found = false;
        for (const next of adj.get(id) ?? []) {
          if (canReach(next, path)) {
            reachable.add(id);
            found = true;
          }
        }
        memo.set(id, found);
        path.delete(id);
        return found;
      };

      canReach(startId, new Set());
      return reachable;
    };

    const downstreamLive = walkReachable(selectedBlockId, downstream, 'output');
    const upstreamLive = walkReachable(selectedBlockId, upstream, 'input');

    const allLive = new Set([...downstreamLive, ...upstreamLive]);
    connections.forEach((conn, i) => {
      if (allLive.has(conn.sourceId) && allLive.has(conn.destId)) live.add(i);
    });
    return live;
  }, [blocks, connections, selectedBlockId]);

  const hasSelection = selectedBlockId !== null;
  const blockMap = new Map(blocks.map((b) => [b.id, b]));

  // Group connections by source block (output side) and dest block (input side)
  // to determine count and sorted index for Y-position spacing.
  const { outputGroups, inputGroups } = useMemo(() => {
    const outGroups = new Map<
      string,
      { destId: string; destRow: number; destCol: number; connIdx: number }[]
    >();
    const inGroups = new Map<
      string,
      { sourceId: string; sourceRow: number; sourceCol: number; connIdx: number }[]
    >();

    connections.forEach((conn, i) => {
      const src = blocks.find((b) => b.id === conn.sourceId);
      const dst = blocks.find((b) => b.id === conn.destId);
      if (!src || !dst) return;

      const outGroup = outGroups.get(conn.sourceId) ?? [];
      outGroup.push({ destId: conn.destId, destRow: dst.row, destCol: dst.col, connIdx: i });
      outGroups.set(conn.sourceId, outGroup);

      const inGroup = inGroups.get(conn.destId) ?? [];
      inGroup.push({ sourceId: conn.sourceId, sourceRow: src.row, sourceCol: src.col, connIdx: i });
      inGroups.set(conn.destId, inGroup);
    });

    for (const group of outGroups.values())
      group.sort((a, b) => a.destRow - b.destRow || a.destCol - b.destCol);
    for (const group of inGroups.values())
      group.sort((a, b) => a.sourceRow - b.sourceRow || a.sourceCol - b.sourceCol);

    return { outputGroups: outGroups, inputGroups: inGroups };
  }, [connections, blocks]);

  const gw = gridWidth(grid.columns);
  const gh = gridHeight(grid.rows);

  return (
    <svg className={styles.svg} style={{ width: gw, height: gh }}>
      {connections.map((conn, i) => {
        const src = blockMap.get(conn.sourceId);
        const dst = blockMap.get(conn.destId);
        if (!src || !dst) return null;

        const outGroup = outputGroups.get(conn.sourceId) ?? [];
        const inGroup = inputGroups.get(conn.destId) ?? [];
        const outIdx = outGroup.findIndex((g) => g.connIdx === i);
        const inIdx = inGroup.findIndex((g) => g.connIdx === i);

        const x1 = outputPortX(src.col);
        const y1 = connectionY(src.row, outGroup.length, outIdx);
        const x2 = inputPortX(dst.col);
        const y2 = connectionY(dst.row, inGroup.length, inIdx);

        const wireColor = wireColors[i % wireColors.length];
        const isLive = liveConnections.has(i);
        const stroke = hasSelection
          ? isLive
            ? colors.warning
            : wireColor + '26'
          : wireColor + 'bb';

        const d = orthogonalPath(x1, y1, x2, y2);

        return (
          <g key={i}>
            {/* Invisible wide hit area for clicking */}
            <path
              d={d}
              stroke="transparent"
              strokeWidth={12}
              fill="none"
              style={{ pointerEvents: 'stroke', cursor: 'pointer' }}
              onMouseEnter={() => setHoveredConn(i)}
              onMouseLeave={() => setHoveredConn((prev) => (prev === i ? null : prev))}
              onClick={(e) => {
                e.stopPropagation();
                onConnectionClick?.(e, conn.sourceId, conn.destId);
              }}
            />
            {/* Visible connection line */}
            <path
              d={d}
              stroke={stroke}
              strokeDasharray={hoveredConn === i ? '6 4' : undefined}
              strokeWidth={2}
              fill="none"
              style={{ pointerEvents: 'none' }}
            />
          </g>
        );
      })}

      {dragging &&
        (() => {
          const blk = blockMap.get(dragging.blockId);
          if (!blk) return null;
          const x1 = dragging.portType === 'output' ? outputPortX(blk.col) : inputPortX(blk.col);
          const y1 = connectionY(blk.row, 1, 0);
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
