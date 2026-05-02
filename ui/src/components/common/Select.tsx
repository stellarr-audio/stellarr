import { Select as RadixSelect } from 'radix-ui';
import { ChevronDownIcon } from '@radix-ui/react-icons';
import styles from './Select.module.css';

export interface SelectOption {
  value: string;
  label: string;
}

interface Props {
  value: string;
  onValueChange: (value: string) => void;
  options: SelectOption[];
  ariaLabel?: string;
  /** Bumps z-index above modal dialogs. Set when this Select is rendered
      inside a `<Dialog>` so the dropdown surfaces above the dialog content. */
  inDialog?: boolean;
  /** Optional override for the trigger className. */
  triggerClassName?: string;
}

export function Select({
  value,
  onValueChange,
  options,
  ariaLabel,
  inDialog,
  triggerClassName,
}: Props) {
  const triggerCls = [styles.trigger, triggerClassName].filter(Boolean).join(' ');
  const contentCls = [styles.content, inDialog && styles.inDialog].filter(Boolean).join(' ');
  return (
    <RadixSelect.Root value={value} onValueChange={onValueChange}>
      <RadixSelect.Trigger className={triggerCls} aria-label={ariaLabel}>
        <RadixSelect.Value />
        <RadixSelect.Icon>
          <ChevronDownIcon />
        </RadixSelect.Icon>
      </RadixSelect.Trigger>
      <RadixSelect.Portal>
        <RadixSelect.Content position="popper" sideOffset={4} className={contentCls}>
          <RadixSelect.Viewport>
            {options.map((o) => (
              <RadixSelect.Item key={o.value} value={o.value} className={styles.item}>
                <RadixSelect.ItemText>{o.label}</RadixSelect.ItemText>
              </RadixSelect.Item>
            ))}
          </RadixSelect.Viewport>
        </RadixSelect.Content>
      </RadixSelect.Portal>
    </RadixSelect.Root>
  );
}
