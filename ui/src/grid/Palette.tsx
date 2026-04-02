import { colors } from './colors';

const blockTypes = [
  { type: 'input', label: 'Input' },
  { type: 'output', label: 'Output' },
  { type: 'amp', label: 'Amp' },
  { type: 'cab', label: 'Cab' },
  { type: 'fx', label: 'FX' },
];

export function Palette() {
  return (
    <div
      style={{
        display: 'flex',
        gap: '0.5rem',
        padding: '0.5rem 0',
      }}
    >
      <span
        style={{
          color: colors.muted,
          fontSize: '1rem',
          letterSpacing: '0.1em',
          textTransform: 'uppercase',
          alignSelf: 'center',
          marginRight: '0.25rem',
        }}
      >
        Blocks
      </span>
      {blockTypes.map((bt) => (
        <div
          key={bt.type}
          draggable
          onDragStart={(e) => {
            e.dataTransfer.setData('blockType', bt.type);
            e.dataTransfer.effectAllowed = 'copy';
          }}
          style={{
            background: colors.blockBg,
            border: `1px solid ${colors.blockBorder}`,
            color: colors.text,
            fontSize: '1rem',
            fontWeight: 600,
            letterSpacing: '0.05em',
            padding: '0.3rem 0.7rem',
            cursor: 'grab',
            userSelect: 'none',
            textTransform: 'uppercase',
          }}
        >
          {bt.label}
        </div>
      ))}
    </div>
  );
}
