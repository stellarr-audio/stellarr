import { useMemo } from 'react';
import {
  ReactFlow,
  ReactFlowProvider,
  Background,
  BackgroundVariant,
  Controls,
  type Node,
  type Edge,
  type NodeTypes,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { useStore } from '../../store';
import { BlockNode, type BlockNodeData } from './BlockNode';
import styles from './Canvas.module.css';

const CELL_SIZE = 80;
const SNAP: [number, number] = [20, 20];

const nodeTypes: NodeTypes = {
  block: BlockNode,
};

function CanvasInner() {
  const blocks = useStore((s) => s.blocks);
  const connections = useStore((s) => s.connections);

  const nodes = useMemo<Node<BlockNodeData>[]>(
    () =>
      blocks.map((block) => ({
        id: block.id,
        type: 'block',
        position: { x: block.col * CELL_SIZE * 2, y: block.row * CELL_SIZE },
        data: { block },
      })),
    [blocks],
  );

  const edges = useMemo<Edge[]>(
    () =>
      connections.map((c) => ({
        id: `${c.sourceId}-${c.destId}`,
        source: c.sourceId,
        target: c.destId,
        sourceHandle: 'out-top',
        targetHandle: 'in-top',
        type: 'default',
      })),
    [connections],
  );

  return (
    <div className={styles.wrapper}>
      <ReactFlow
        nodes={nodes}
        edges={edges}
        nodeTypes={nodeTypes}
        fitView
        snapToGrid
        snapGrid={SNAP}
        proOptions={{ hideAttribution: true }}
      >
        <Background variant={BackgroundVariant.Dots} gap={20} size={1} />
        <Controls showInteractive={false} position="top-right" />
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
