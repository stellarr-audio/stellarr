import { Dialog } from 'radix-ui';
import styles from './SceneRenameDialog.module.css';

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
        <Dialog.Overlay className={styles.overlay} />
        <Dialog.Content className={styles.content}>
          <Dialog.Title className={styles.title}>Rename Scene</Dialog.Title>
          <input
            autoFocus
            value={value}
            onChange={(e) => onChange(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter') onSubmit();
            }}
            className={styles.input}
          />
          <div className={styles.buttonRow}>
            <button onClick={() => onOpenChange(false)} className={styles.cancelButton}>
              Cancel
            </button>
            <button onClick={onSubmit} className={styles.renameButton}>
              Rename
            </button>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
