import { colors } from './colors';

interface Props {
  enabled: boolean;
  onToggle: () => void;
  title?: string;
}

export function ToggleSwitch({ enabled, onToggle, title }: Props) {
  return (
    <button
      onClick={onToggle}
      title={title}
      style={{
        position: 'relative',
        width: 36,
        height: 20,
        borderRadius: 10,
        border: 'none',
        background: enabled ? colors.green : colors.border,
        cursor: 'pointer',
        padding: 0,
        transition: 'background 0.2s ease',
      }}
    >
      <div
        style={{
          position: 'absolute',
          top: 2,
          left: enabled ? 18 : 2,
          width: 16,
          height: 16,
          borderRadius: '50%',
          background: '#ffffff',
          transition: 'left 0.2s ease',
        }}
      />
    </button>
  );
}
