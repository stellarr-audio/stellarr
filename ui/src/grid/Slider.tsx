import { Slider as RadixSlider } from 'radix-ui';
import { colors } from './colors';

interface Props {
  value: number;
  min?: number;
  max?: number;
  step?: number;
  onChange: (value: number) => void;
}

export function Slider({ value, min = 0, max = 100, step = 1, onChange }: Props) {
  return (
    <RadixSlider.Root
      value={[value]}
      onValueChange={([v]) => onChange(v)}
      min={min}
      max={max}
      step={step}
      style={{
        position: 'relative',
        display: 'flex',
        alignItems: 'center',
        width: '100%',
        height: 20,
        userSelect: 'none',
        touchAction: 'none',
      }}
    >
      <RadixSlider.Track
        style={{
          position: 'relative',
          flexGrow: 1,
          height: 6,
          borderRadius: 9999,
          background: colors.border,
          overflow: 'hidden',
        }}
      >
        <RadixSlider.Range
          style={{
            position: 'absolute',
            height: '100%',
            borderRadius: 9999,
            background: colors.primary,
          }}
        />
      </RadixSlider.Track>
      <RadixSlider.Thumb
        style={{
          display: 'block',
          width: 16,
          height: 16,
          borderRadius: 9999,
          background: colors.secondary,
          boxShadow: `0 0 4px ${colors.secondary}44`,
          outline: 'none',
          cursor: 'grab',
        }}
      />
    </RadixSlider.Root>
  );
}
