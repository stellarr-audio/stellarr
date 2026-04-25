import { Popover } from 'radix-ui';
import { blockPalette } from '../common/colors';
import styles from './ColorPicker.module.css';

interface Props {
  color?: string;
  onChange: (color: string) => void;
}

export function ColorPicker({ color, onChange }: Props) {
  const currentColor = color || blockPalette[0];

  return (
    <Popover.Root>
      <Popover.Trigger asChild>
        <button
          className={styles.trigger}
          style={{ background: currentColor }}
          title="Block color"
        />
      </Popover.Trigger>
      <Popover.Portal>
        <Popover.Content sideOffset={4} className={styles.content}>
          {blockPalette.map((c) => (
            <button
              key={c}
              onClick={() => onChange(c)}
              className={`${styles.swatch} ${c === currentColor ? styles.swatchSelected : ''}`}
              style={{ background: c }}
              title={c}
            />
          ))}
        </Popover.Content>
      </Popover.Portal>
    </Popover.Root>
  );
}
