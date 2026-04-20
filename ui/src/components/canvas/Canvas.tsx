import { useEffect, useMemo } from 'react';
import {
  ReactFlow,
  ReactFlowProvider,
  Background,
  BackgroundVariant,
  Controls,
  useNodesState,
  useEdgesState,
  type Node,
  type Edge,
  type NodeTypes,
} from '@xyflow/react';
import '@xyflow/react/dist/style.css';
import { useStore } from '../../store';
import { BlockNode, type BlockNodeData } from './BlockNode';
import styles from './Canvas.module.css';

const COL_STEP = 180;
const ROW_STEP = 140;
const SNAP: [number, number] = [20, 20];

const nodeTypes: NodeTypes = {
  block: BlockNode,
};

function CanvasInner() {
  const blocks = useStore((s) => s.blocks);
  const connections = useStore((s) => s.connections);

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
