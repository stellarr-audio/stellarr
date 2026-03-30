import { useCallback, useRef, useState } from 'react';
import type { GridBlock as GridBlockData } from '../store';
import {
  requestRemoveBlock,
  requestRemoveConnection,
  requestToggleTestTone,
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
  amp: '#ff8800',
  cab: '#cc66ff',
  fx: '#00ffe0',
  utility: colors.muted,
  vst: colors.secondary,
};

const typeAbbreviations: Record<string, string> = {
  input: 'INP',
  output: 'OUT',
  amp: 'AMP',
  cab: 'CAB',
  fx: 'EFX',
  utility: 'UTL',
  vst: 'VST',
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
        transform: 'translateY(-50%)',
        width: 12,
        height: 12,
        borderRadius: '50%',
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
  const [hovered, setHovered] = useState(false);
  const portActive = useRef(false);

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
      style={{
        position: 'absolute',
        left: cellLeft(block.col),
        top: cellTop(block.row),
        width: CELL_SIZE,
        height: CELL_SIZE,
        background: colors.blockBg,
        border: `1px solid ${accentColor}55`,
        boxSizing: 'border-box',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        userSelect: 'none',
        cursor: 'grab',
        zIndex: 2,
      }}
    >
      <div
        style={{
          fontSize: '0.7rem',
          fontWeight: 700,
          color: accentColor,
          letterSpacing: '0.12em',
        }}
      >
        {typeAbbreviations[block.type] ?? block.type.slice(0, 3).toUpperCase()}
      </div>
      {block.type !== 'input' && block.type !== 'output' && (
        <div
          style={{
            fontSize: '0.4rem',
            fontWeight: 600,
            color: colors.muted,
            letterSpacing: '0.08em',
            textTransform: 'uppercase',
            marginTop: '0.15rem',
          }}
        >
          {block.name}
        </div>
      )}

      {/* Input port (left) */}
      {block.type !== 'input' && (
        <Port side="input" blockId={block.id} connected={hasInputConnection} />
      )}

      {/* Output port (right) */}
      {block.type !== 'output' && (
        <Port side="output" blockId={block.id} connected={hasOutputConnection} />
      )}

      {/* Test tone toggle (Input blocks only) */}
      {block.type === 'input' && (
        <div
          onClick={() => requestToggleTestTone(block.id)}
          style={{
            position: 'absolute',
            bottom: 4,
            left: '50%',
            transform: 'translateX(-50%)',
            fontSize: '0.45rem',
            fontWeight: 600,
            letterSpacing: '0.05em',
            textTransform: 'uppercase',
            color: block.testTone ? colors.green : colors.muted,
            cursor: 'pointer',
            whiteSpace: 'nowrap',
          }}
        >
          {block.testTone ? 'Tone On' : 'Test Tone'}
        </div>
      )}

      {/* Remove button — hover only */}
      {hovered && (
        <div
          onClick={(e) => {
            e.stopPropagation();
            requestRemoveBlock(block.id);
          }}
          style={{
            position: 'absolute',
            top: 3,
            right: 5,
            width: 14,
            height: 14,
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            fontSize: '0.6rem',
            fontWeight: 700,
            color: colors.muted,
            cursor: 'pointer',
            lineHeight: 1,
          }}
        >
          x
        </div>
      )}
    </div>
  );
}
