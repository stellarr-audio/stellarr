import { useCallback, useRef } from 'react';
import { Cross2Icon, SpeakerLoudIcon } from '@radix-ui/react-icons';
import type { GridBlock as GridBlockData } from '../../store';
import { requestRemoveBlock, requestRemoveConnection, requestOpenPluginEditor } from '../../bridge';
import { useStore } from '../../store';
import { colors } from '../common/colors';
import { CELL_SIZE, cellLeft, cellTop } from './layout';
import styles from './GridBlock.module.css';

interface Props {
  block: GridBlockData;
}

const typeColors: Record<string, string> = {
  input: colors.green,
  output: colors.primary,
  plugin: colors.secondary,
};

const typeAbbreviations: Record<string, string> = {
  input: 'INP',
  output: 'OUT',
  plugin: 'PLG',
};

function Port({
  side,
  blockId,
  connected,
}: {
  side: 'input' | 'output';
  blockId: string;
  connected: boolean;
}) {
  const connections = useStore((s) => s.connections);
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);

  const isLeft = side === 'input';

  const handleClick = useCallback(() => {
    if (!connected) return;

    const conn = isLeft
      ? connections.find((c) => c.destId === blockId)
      : connections.find((c) => c.sourceId === blockId);

    if (conn) requestRemoveConnection(conn.sourceId, conn.destId);
  }, [connected, connections, blockId, isLeft]);

  const handleMouseDown = useCallback(
    (e: React.MouseEvent) => {
      if (connected) return; // connected ports use click-to-disconnect instead
      e.stopPropagation();
      e.preventDefault();
      setDraggingConnection({
        blockId,
        portType: side,
        mouseX: e.clientX,
        mouseY: e.clientY,
      });
    },
    [blockId, side, connected, setDraggingConnection],
  );

  const portClassName = `${styles.port} ${isLeft ? styles.portLeft : styles.portRight} ${connected ? styles.portConnected : styles.portDisconnected}`;

  return (
    <div
      draggable={false}
      data-port-block-id={blockId}
      data-port-type={side}
      onMouseDown={handleMouseDown}
      onDragStart={(e) => e.preventDefault()}
      onClick={handleClick}
      className={portClassName}
    />
  );
}

export function GridBlockComponent({ block }: Props) {
  const connections = useStore((s) => s.connections);
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const selectBlock = useStore((s) => s.selectBlock);
  const portActive = useRef(false);

  const isSelected = selectedBlockId === block.id;
  const hasInputConnection = connections.some((c) => c.destId === block.id);
  const hasOutputConnection = connections.some((c) => c.sourceId === block.id);

  const handleDragStart = useCallback(
    (e: React.DragEvent) => {
      if (portActive.current) {
        e.preventDefault();
        portActive.current = false;
        return;
      }
      e.dataTransfer.setData('moveBlockId', block.id);
      e.dataTransfer.effectAllowed = 'move';
    },
    [block.id],
  );

  const accentColor = typeColors[block.type] ?? colors.secondary;

  // Border must stay inline — it depends on multiple dynamic values (isSelected, bypassed, accentColor)
  const borderStyle = isSelected
    ? `2px ${block.bypassed ? 'dashed' : 'solid'} #ffffff`
    : block.bypassed
      ? `2px dashed ${accentColor}44`
      : `1px solid ${accentColor}55`;

  return (
    <div
      draggable
      onDragStart={handleDragStart}
      onMouseDown={() => {
        portActive.current = false;
      }}
      onClick={() => selectBlock(block.id)}
      onDoubleClick={() => {
        if (block.type === 'plugin' && block.pluginId) requestOpenPluginEditor(block.id);
      }}
      className={styles.block}
      style={{
        left: cellLeft(block.col),
        top: cellTop(block.row),
        width: CELL_SIZE,
        height: CELL_SIZE,
        border: borderStyle,
      }}
    >
      {/* Top region — status icons / format tag */}
      <div className={styles.topRegion}>
        {block.type === 'input' && block.testTone && (
          <SpeakerLoudIcon width={12} height={12} color={colors.green} />
        )}
        {block.type === 'plugin' && block.pluginFormat && (
          <span className={styles.formatTag}>
            {block.pluginFormat === 'AudioUnit' ? 'AU' : block.pluginFormat}
          </span>
        )}
      </div>

      {/* Middle region — block type */}
      <div className={styles.blockType} style={{ color: accentColor }}>
        {block.displayName || typeAbbreviations[block.type] || block.type.slice(0, 3).toUpperCase()}
      </div>

      {/* Bottom region — subtitle */}
      <div className={styles.bottomRegion}>
        {block.type === 'plugin' && (
          <div className={styles.pluginName}>{block.pluginName ?? 'No plugin'}</div>
        )}
      </div>

      {/* Input port (left) */}
      {block.type !== 'input' && (
        <Port side="input" blockId={block.id} connected={hasInputConnection} />
      )}

      {/* Output port (right) */}
      {block.type !== 'output' && (
        <Port side="output" blockId={block.id} connected={hasOutputConnection} />
      )}

      {/* Remove button — visible on block hover via CSS */}
      <div
        className={styles.removeButton}
        onClick={(e) => {
          e.stopPropagation();
          requestRemoveBlock(block.id);
        }}
      >
        <Cross2Icon width={14} height={14} />
      </div>
    </div>
  );
}
