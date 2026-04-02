import { useCallback, useRef, useState } from 'react';
import { Cross2Icon, SpeakerLoudIcon } from '@radix-ui/react-icons';
import type { GridBlock as GridBlockData } from '../store';
import {
  requestRemoveBlock,
  requestRemoveConnection,
  requestOpenPluginEditor,
} from '../bridge';
import { useStore } from '../store';
import { colors } from './colors';
import { CELL_SIZE, cellLeft, cellTop } from './layout';

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
  const [hovered, setHovered] = useState(false);
  const connections = useStore((s) => s.connections);
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);

  const isLeft = side === 'input';
  const showDisconnect = connected && hovered;

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

  return (
    <div
      draggable={false}
      data-port-block-id={blockId}
      data-port-type={side}
      onMouseDown={handleMouseDown}
      onDragStart={(e) => e.preventDefault()}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
      onClick={handleClick}
      style={{
        position: 'absolute',
        [isLeft ? 'left' : 'right']: -6,
        top: '50%',
        transform: 'translateY(-50%) rotate(45deg)',
        width: 12,
        height: 12,
        background: showDisconnect
          ? '#ff4444'
          : !connected && hovered
            ? '#ffcc00'
            : colors.portColor,
        cursor: connected ? 'pointer' : 'crosshair',
        zIndex: 3,
        transition: 'background 0.15s ease',
      }}
    />
  );
}

export function GridBlockComponent({ block }: Props) {
  const connections = useStore((s) => s.connections);
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const selectBlock = useStore((s) => s.selectBlock);
  const [hovered, setHovered] = useState(false);
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

  return (
    <div
      draggable
      onDragStart={handleDragStart}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
      onMouseDown={() => { portActive.current = false; }}
      onClick={() => selectBlock(block.id)}
      onDoubleClick={() => {
        if (block.type === 'plugin' && block.pluginId)
          requestOpenPluginEditor(block.id);
      }}
      style={{
        position: 'absolute',
        left: cellLeft(block.col),
        top: cellTop(block.row),
        width: CELL_SIZE,
        height: CELL_SIZE,
        background: colors.blockBg,
        border: isSelected
          ? `2px ${block.bypassed ? 'dashed' : 'solid'} #ffffff`
          : block.bypassed
            ? `2px dashed ${accentColor}44`
            : `1px solid ${accentColor}55`,
        boxSizing: 'border-box',
        display: 'grid',
        gridTemplateRows: '1fr auto 1fr',
        alignItems: 'center',
        justifyItems: 'center',
        overflow: 'hidden',
        userSelect: 'none',
        cursor: 'grab',
        zIndex: 2,
        padding: '4px 2px',
      }}
    >
      {/* Top region — status icons */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        {block.type === 'input' && block.testTone && (
          <SpeakerLoudIcon width={12} height={12} color={colors.green} />
        )}
      </div>

      {/* Middle region — block type */}
      <div
        style={{
          fontSize: '1rem',
          fontWeight: 700,
          color: accentColor,
          letterSpacing: '0.12em',
        }}
      >
        {typeAbbreviations[block.type] ?? block.type.slice(0, 3).toUpperCase()}
      </div>

      {/* Bottom region — subtitle */}
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          minHeight: 16,
          width: '100%',
          overflow: 'hidden',
        }}
      >
        {block.type === 'plugin' && (
          <div
            style={{
              fontSize: '0.85rem',
              fontWeight: 600,
              color: colors.muted,
              letterSpacing: '0.05em',
              textTransform: 'uppercase',
              overflow: 'hidden',
              textOverflow: 'ellipsis',
              whiteSpace: 'nowrap',
              textAlign: 'center',
            }}
          >
            {block.pluginName ?? 'No plugin'}
          </div>
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

      {/* Remove button — hover only, floats above top-right corner */}
      {hovered && (
        <div
          onClick={(e) => {
            e.stopPropagation();
            requestRemoveBlock(block.id);
          }}
          style={{
            position: 'absolute',
            top: 3,
            right: 3,
            color: '#cc2222',
            cursor: 'pointer',
            zIndex: 5,
          }}
        >
          <Cross2Icon width={14} height={14} />
        </div>
      )}
    </div>
  );
}
