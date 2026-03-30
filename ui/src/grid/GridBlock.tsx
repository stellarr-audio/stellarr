import { useCallback } from 'react';
import type { GridBlock as GridBlockData } from '../store';
import { requestRemoveBlock, requestToggleTestTone } from '../bridge';
import { useStore } from '../store';
import { colors } from './colors';

interface Props {
  block: GridBlockData;
  cellSize: number;
}

const typeColors: Record<string, string> = {
  input: colors.green,
  output: colors.primary,
  gain: colors.secondary,
};

export function GridBlockComponent({ block, cellSize }: Props) {
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);

  const handlePortMouseDown = useCallback(
    (e: React.MouseEvent) => {
      e.stopPropagation();
      setDraggingConnection({
        sourceId: block.id,
        mouseX: e.clientX,
        mouseY: e.clientY,
      });
    },
    [block.id, setDraggingConnection],
  );

  const accentColor = typeColors[block.type] ?? colors.secondary;

  return (
    <div
      style={{
        position: 'absolute',
        left: block.col * cellSize,
        top: block.row * cellSize,
        width: cellSize - 2,
        height: cellSize - 2,
        background: colors.blockBg,
        border: `1px solid ${accentColor}44`,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        userSelect: 'none',
        zIndex: 2,
      }}
    >
      <div
        style={{
          fontSize: '0.55rem',
          fontWeight: 700,
          color: accentColor,
          letterSpacing: '0.08em',
          textTransform: 'uppercase',
          marginBottom: '0.15rem',
        }}
      >
        {block.type}
      </div>
      <div
        style={{
          fontSize: '0.5rem',
          color: colors.muted,
        }}
      >
        {block.name}
      </div>

      {/* Input port (left) */}
      {block.type !== 'input' && (
        <div
          data-port-block-id={block.id}
          data-port-type="input"
          style={{
            position: 'absolute',
            left: -5,
            top: '50%',
            transform: 'translateY(-50%)',
            width: 10,
            height: 10,
            borderRadius: '50%',
            background: colors.portColor,
            cursor: 'pointer',
            zIndex: 3,
          }}
        />
      )}

      {/* Output port (right) */}
      {block.type !== 'output' && (
        <div
          onMouseDown={handlePortMouseDown}
          style={{
            position: 'absolute',
            right: -5,
            top: '50%',
            transform: 'translateY(-50%)',
            width: 10,
            height: 10,
            borderRadius: '50%',
            background: colors.portColor,
            cursor: 'crosshair',
            zIndex: 3,
          }}
        />
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

      {/* Remove button */}
      {block.type !== 'input' && block.type !== 'output' && (
        <div
          onClick={() => requestRemoveBlock(block.id)}
          style={{
            position: 'absolute',
            top: 2,
            right: 4,
            fontSize: '0.55rem',
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
