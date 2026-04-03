import { Popover } from 'radix-ui';
import styles from './ColorPicker.module.css';

// 8 base hues × 2 shades (regular + light) = 16 swatches
export const BLOCK_COLORS = [
  '#0077cc',
  '#38bdf8', // blue
  '#6c3fc7',
  '#a78bfa', // violet
  '#cc1259',
  '#f472b6', // pink
  '#cc5500',
  '#ff8800', // orange
  '#b8960a',
  '#ffd43b', // gold
  '#00995c',
  '#00ff9d', // green
  '#008c8c',
  '#00d2d3', // teal
  '#5a6578',
  '#94a3b8', // slate
];

export const PALETTE = {
  blue: BLOCK_COLORS[0],
  blueLight: BLOCK_COLORS[1],
  violet: BLOCK_COLORS[2],
  violetLight: BLOCK_COLORS[3],
  pink: BLOCK_COLORS[4],
  pinkLight: BLOCK_COLORS[5],
  orange: BLOCK_COLORS[6],
  orangeLight: BLOCK_COLORS[7],
  gold: BLOCK_COLORS[8],
  goldLight: BLOCK_COLORS[9],
  green: BLOCK_COLORS[10],
  greenLight: BLOCK_COLORS[11],
  teal: BLOCK_COLORS[12],
  tealLight: BLOCK_COLORS[13],
  slate: BLOCK_COLORS[14],
  slateLight: BLOCK_COLORS[15],
};

interface Props {
  color?: string;
  onChange: (color: string) => void;
}

export function ColorPicker({ color, onChange }: Props) {
  const currentColor = color || BLOCK_COLORS[0];

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
          {BLOCK_COLORS.map((c) => (
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
