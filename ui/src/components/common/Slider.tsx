import { Slider as RadixSlider } from 'radix-ui';
import styles from './Slider.module.css';

interface Props {
  value: number;
  min?: number;
  max?: number;
  step?: number;
  defaultValue?: number;
  onChange: (value: number) => void;
}

export function Slider({ value, min = 0, max = 100, step = 1, defaultValue = 0, onChange }: Props) {
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
        <RadixSlider.Range className={styles.range} />
      </RadixSlider.Track>
      <RadixSlider.Thumb className={styles.thumb} />
    </RadixSlider.Root>
  );
}
