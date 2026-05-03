import { Slider as RadixSlider } from 'radix-ui';
import styles from './Slider.module.css';

interface Props {
  value: number;
  min?: number;
  max?: number;
  step?: number;
  defaultValue?: number;
  onChange: (value: number) => void;
  fillSide?: 'left' | 'right';
  ariaLabel?: string;
}

export function Slider({
  value,
  min = 0,
  max = 100,
  step = 1,
  defaultValue = 0,
  onChange,
  fillSide = 'left',
  ariaLabel,
}: Props) {
  const pct = max > min ? ((value - min) / (max - min)) * 100 : 0;

  return (
    <RadixSlider.Root
      onDoubleClick={() => onChange(defaultValue)}
      value={[value]}
      onValueChange={([v]) => onChange(v)}
      min={min}
      max={max}
      step={step}
      className={styles.root}
    >
      <RadixSlider.Track className={styles.track}>
        {fillSide === 'left' ? (
          <RadixSlider.Range className={styles.range} />
        ) : (
          <div className={styles.rangeRight} style={{ left: `${pct}%`, right: 0 }} />
        )}
      </RadixSlider.Track>
      <RadixSlider.Thumb className={styles.thumb} aria-label={ariaLabel} />
    </RadixSlider.Root>
  );
}
