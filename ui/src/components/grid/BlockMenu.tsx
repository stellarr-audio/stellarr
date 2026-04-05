import { DropdownMenu } from 'radix-ui';
import styles from './BlockMenu.module.css';

const menuItems = [
  { type: 'input', label: 'Input' },
  { type: 'output', label: 'Output' },
  { type: 'plugin', label: 'Plugin' },
];

interface Props {
  open: boolean;
  onSelect: (type: string) => void;
  onPaste?: () => void;
  onClose: () => void;
  children: React.ReactNode;
}

export function BlockMenu({ open, onSelect, onPaste, onClose, children }: Props) {
  return (
    <DropdownMenu.Root
      open={open}
      onOpenChange={(o) => {
        if (!o) onClose();
      }}
    >
      <DropdownMenu.Trigger asChild>{children}</DropdownMenu.Trigger>

      <DropdownMenu.Portal>
        <DropdownMenu.Content sideOffset={4} className={styles.content}>
          {menuItems.map((item) => (
            <DropdownMenu.Item
              key={item.type}
              onSelect={() => onSelect(item.type)}
              className={styles.item}
            >
              {item.label}
            </DropdownMenu.Item>
          ))}
          {onPaste && (
            <>
              <DropdownMenu.Separator className={styles.separator} />
              <DropdownMenu.Item onSelect={onPaste} className={styles.item}>
                Paste
              </DropdownMenu.Item>
            </>
          )}
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}
