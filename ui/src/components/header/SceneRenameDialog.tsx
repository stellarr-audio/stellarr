import { Dialog } from 'radix-ui';
import { colors } from '../common/colors';

interface Props {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  value: string;
  onChange: (value: string) => void;
  onSubmit: () => void;
}

export function SceneRenameDialog({ open, onOpenChange, value, onChange, onSubmit }: Props) {
  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay
          style={{
            position: 'fixed',
            inset: 0,
            background: 'rgba(0,0,0,0.5)',
            zIndex: 50,
          }}
        />
        <Dialog.Content
          style={{
            position: 'fixed',
            top: '50%',
            left: '50%',
            transform: 'translate(-50%, -50%)',
            background: '#1a1535',
            border: `1px solid ${colors.border}`,
            padding: '1.5rem',
            zIndex: 51,
            minWidth: 280,
            display: 'flex',
            flexDirection: 'column',
            gap: '1rem',
          }}
        >
          <Dialog.Title
            style={{
              fontSize: '1rem',
              fontWeight: 700,
              color: colors.text,
              letterSpacing: '0.06em',
              textTransform: 'uppercase',
              margin: 0,
            }}
          >
            Rename Scene
          </Dialog.Title>
          <input
            autoFocus
            value={value}
            onChange={(e) => onChange(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter') onSubmit();
            }}
            style={{
              background: colors.bg,
              border: `1px solid ${colors.border}`,
              color: colors.text,
              fontSize: '1rem',
              padding: '0.5rem',
              outline: 'none',
            }}
          />
          <div style={{ display: 'flex', gap: '0.5rem', justifyContent: 'flex-end' }}>
            <button
              onClick={() => onOpenChange(false)}
              style={{
                background: 'transparent',
                border: `1px solid ${colors.border}`,
                color: colors.muted,
                padding: '0.35rem 0.75rem',
                fontSize: '0.85rem',
                cursor: 'pointer',
              }}
            >
              Cancel
            </button>
            <button
              onClick={onSubmit}
              style={{
                background: colors.primary,
                border: 'none',
                color: '#ffffff',
                padding: '0.35rem 0.75rem',
                fontSize: '0.85rem',
                fontWeight: 600,
                cursor: 'pointer',
              }}
            >
              Rename
            </button>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
