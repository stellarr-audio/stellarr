import { useRef } from 'react';
import { Dialog } from 'radix-ui';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { sanitiseFilesystemName } from '../../utils/filename';
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
  const inputRef = useRef<HTMLInputElement>(null);

  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay className={styles.overlay} />
        <Dialog.Content
          className={styles.content}
          // Radix's default focuses Dialog.Content (the wrapping <div>). We
          // want the text input focused with the caret at the end of the
          // current name so the user can extend or backspace immediately.
          // Two layers fight for selection on a freshly-mounted input —
          // WebKit's select-all-on-focus and Radix FocusScope's own focus
          // pass — so we both preventDefault Radix's default AND defer our
          // own focus + collapse to the next animation frame so it lands
          // after both. Without the rAF the explicit setSelectionRange runs
          // first and the later focus pass re-selects the whole value.
          onOpenAutoFocus={(e) => {
            e.preventDefault();
            requestAnimationFrame(() => {
              const el = inputRef.current;
              if (el) {
                el.focus();
                const end = el.value.length;
                el.setSelectionRange(end, end);
              }
            });
          }}
        >
          <Dialog.Title className={styles.title}>{title}</Dialog.Title>
          <Input
            ref={inputRef}
            value={value}
            onChange={(e) => onChange(sanitiseFilesystemName(e.target.value))}
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
