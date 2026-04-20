import { memo } from 'react';
import { Handle, Position, type NodeProps } from '@xyflow/react';
import { TYPE_ABBREVIATIONS } from '../common/constants';
import type { GridBlock } from '../../store';
import styles from './BlockNode.module.css';

export interface BlockNodeData extends Record<string, unknown> {
  block: GridBlock;
}

function BlockNodeInner({ data }: NodeProps) {
  const { block } = data as BlockNodeData;
  const isAnchor = block.type === 'input' || block.type === 'output';
  const label =
    block.displayName || block.pluginName || TYPE_ABBREVIATIONS[block.type] || block.type;
  const vendor = block.pluginFormat || (isAnchor ? 'Stellarr' : '');

  const style = block.blockColor
    ? ({ ['--bc']: block.blockColor } as React.CSSProperties)
    : undefined;

  return (
    <div className={`${styles.block} ${isAnchor ? styles.anchor : ''}`} style={style}>
      <div className={styles.head}>
        <span className={styles.name}>{label.slice(0, 10).toUpperCase()}</span>
      </div>
      {vendor && <span className={styles.vendor}>{vendor.slice(0, 10).toUpperCase()}</span>}

      {block.type !== 'input' && (
        <>
          <Handle type="target" position={Position.Left} id="in-top" style={{ top: 14 }} />
          <Handle
            type="target"
            position={Position.Left}
            id="in-bot"
            style={{ top: 'auto', bottom: 10 }}
          />
        </>
      )}
      {block.type !== 'output' && (
        <>
          <Handle type="source" position={Position.Right} id="out-top" style={{ top: 14 }} />
          <Handle
            type="source"
            position={Position.Right}
            id="out-bot"
            style={{ top: 'auto', bottom: 10 }}
          />
        </>
      )}
    </div>
  );
}

export const BlockNode = memo(BlockNodeInner);
