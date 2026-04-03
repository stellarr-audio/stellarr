import { Cross2Icon } from '@radix-ui/react-icons';
import { colors } from '../common/colors';

interface Props {
  index: number;
  isActive: boolean;
  isDirty: boolean;
  canDelete: boolean;
  onRecall: () => void;
  onDelete: () => void;
}

export function StateSquare({ index, isActive, isDirty, canDelete, onRecall, onDelete }: Props) {
  const borderStyle = isActive ? `2px solid #ffffff` : `1px solid ${colors.border}`;

  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'stretch',
        border: borderStyle,
      }}
    >
      <button
        onClick={() => {
          if (!isActive) onRecall();
        }}
        title={isActive ? `State ${index + 1} (active)` : `Recall state ${index + 1}`}
        style={{
          width: 28,
          height: 28,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          background: isDirty ? '#ffaa004d' : isActive ? `${colors.secondary}22` : 'transparent',
          border: 'none',
          color: '#ffffff',
          fontSize: '0.9rem',
          fontWeight: 700,
          cursor: 'pointer',
          padding: 0,
        }}
      >
        {index + 1}
      </button>
      {canDelete && (
        <button
          onClick={onDelete}
          title={`Delete State ${index + 1}`}
          style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            background: 'transparent',
            border: 'none',
            borderLeft: `1px solid ${colors.border}`,
            color: colors.muted,
            padding: '0 0.2rem',
            cursor: 'pointer',
          }}
        >
          <Cross2Icon width={10} height={10} />
        </button>
      )}
    </div>
  );
}
