import { useCallback, useEffect, useMemo } from 'react';
import {
  ReactFlow,
  ReactFlowProvider,
  Background,
  BackgroundVariant,
  Controls,
  ControlButton,
  useNodesState,
  useEdgesState,
  useReactFlow,
  type Node,
  type Edge,
  type NodeTypes,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { TbLayoutGrid } from 'react-icons/tb';
import { useStore } from '../../store';
import { BlockNode, type BlockNodeData } from './BlockNode';
import styles from './Canvas.module.css';

const COL_STEP = 180;
const ROW_STEP = 140;
const SNAP: [number, number] = [20, 20];

// Compact layout: 2-cell gap (40px) between blocks.
const BLOCK_WIDTH = 120;
const BLOCK_HEIGHT = 100;
const COMPACT_GAP = 40;
const COMPACT_STEP_X = BLOCK_WIDTH + COMPACT_GAP;
const COMPACT_STEP_Y = BLOCK_HEIGHT + COMPACT_GAP;

const nodeTypes: NodeTypes = {
  block: BlockNode,
};

/**
 * Reflow the given nodes so visually adjacent blocks are at most 2 small-grid
 * cells apart. Groups by approximate row (vertical bucket), sorts each row by
 * x, then assigns tight-packed positions.
 */
function compactLayout<T extends Node>(nodes: T[]): T[] {
  if (nodes.length === 0) return nodes;

  const sorted = [...nodes].sort((a, b) => a.position.y - b.position.y);
  const rows: T[][] = [];
  let currentRow: T[] = [];
  let currentRowY = -Infinity;
  const rowTolerance = BLOCK_HEIGHT;

  for (const node of sorted) {
    if (node.position.y - currentRowY > rowTolerance) {
      if (currentRow.length) rows.push(currentRow);
      currentRow = [node];
      currentRowY = node.position.y;
    } else {
      currentRow.push(node);
    }
  }
  if (currentRow.length) rows.push(currentRow);

  for (const row of rows) {
    row.sort((a, b) => a.position.x - b.position.x);
  }

  const positions = new Map<string, { x: number; y: number }>();
  rows.forEach((row, rowIdx) => {
    row.forEach((node, colIdx) => {
      positions.set(node.id, { x: colIdx * COMPACT_STEP_X, y: rowIdx * COMPACT_STEP_Y });
    });
  });

  return nodes.map((n) => {
    const p = positions.get(n.id);
    return p ? { ...n, position: p } : n;
  });
}

function CanvasInner() {
  const blocks = useStore((s) => s.blocks);
  const connections = useStore((s) => s.connections);
  const { fitView } = useReactFlow();

  const initialNodes = useMemo<Node<BlockNodeData>[]>(
    () =>
      blocks.map((block) => ({
        id: block.id,
        type: 'block',
        position: { x: block.col * COL_STEP, y: block.row * ROW_STEP },
        data: { block },
      })),
    [blocks],
  );

  const initialEdges = useMemo<Edge[]>(
    () =>
      connections.map((c) => ({
        id: `${c.sourceId}-${c.destId}`,
        source: c.sourceId,
        target: c.destId,
        sourceHandle: 'out-top',
        targetHandle: 'in-top',
      })),
    [connections],
  );

  const [nodes, setNodes, onNodesChange] = useNodesState<Node<BlockNodeData>>(initialNodes);
  const [edges, setEdges, onEdgesChange] = useEdgesState<Edge>(initialEdges);

  useEffect(() => {
    setNodes(initialNodes);
  }, [initialNodes, setNodes]);

  useEffect(() => {
    setEdges(initialEdges);
  }, [initialEdges, setEdges]);

  const handleCompact = useCallback(() => {
    setNodes((current) => compactLayout(current));
    // Give the layout one tick to apply, then refit the viewport around it.
    requestAnimationFrame(() => fitView({ padding: 0.3, minZoom: 1, maxZoom: 1.2 }));
  }, [setNodes, fitView]);

  return (
    <div className={styles.wrapper}>
      <ReactFlow
        nodes={nodes}
        edges={edges}
        onNodesChange={onNodesChange}
        onEdgesChange={onEdgesChange}
        nodeTypes={nodeTypes}
        fitView
        fitViewOptions={{ padding: 0.3, minZoom: 1, maxZoom: 1.2 }}
        snapToGrid
        snapGrid={SNAP}
        proOptions={{ hideAttribution: true }}
      >
        <Background variant={BackgroundVariant.Dots} gap={20} size={1} />
        <Controls showInteractive={false} position="top-right">
          <ControlButton onClick={handleCompact} title="Compact layout">
            <TbLayoutGrid />
          </ControlButton>
        </Controls>
      </ReactFlow>
    </div>
  );
}

export function Canvas() {
  return (
    <ReactFlowProvider>
      <CanvasInner />
    </ReactFlowProvider>
  );
}
