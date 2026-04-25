import { Dialog } from 'radix-ui';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import styles from './SceneRenameDialog.module.css';

interface Props {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  title?: string;
  value: string;
  onChange: (value: string) => void;
  onSubmit: () => void;
}

export function SceneRenameDialog({
  open,
  onOpenChange,
  title = 'Rename Scene',
  value,
  onChange,
  onSubmit,
}: Props) {
  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay className={styles.overlay} />
        <Dialog.Content className={styles.content}>
          <Dialog.Title className={styles.title}>{title}</Dialog.Title>
          <Input
            autoFocus
            value={value}
            onChange={(e) => onChange(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter') onSubmit();
            }}
          />
          <div className={styles.buttonRow}>
            <Button variant="secondary" onClick={() => onOpenChange(false)}>
              Cancel
            </Button>
            <Button onClick={onSubmit}>Rename</Button>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
