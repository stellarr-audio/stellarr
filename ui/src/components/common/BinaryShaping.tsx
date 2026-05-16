import { useEffect, useState } from 'react';
import type { TargetMeta } from './shaping/targetMeta';
import { Numeric } from './Numeric';
import { Slider } from './Slider';
import styles from './BinaryShaping.module.css';

interface Props {
  meta: TargetMeta;
  threshold: number;
  onChange: (next: number) => void;
}

const CC_MIN_THRESHOLD = 1;
const CC_MAX_THRESHOLD = 127;
// Midpoint of the valid CC threshold range — used as the double-click reset
// target on the shared Slider so resets land in-range rather than at 0.
const CC_DEFAULT_THRESHOLD = 64;

export function BinaryShaping({ meta, threshold, onChange }: Props) {
  if (meta.kind !== 'binary' || !meta.binaryLabels || !meta.binaryHelpTemplate) {
    return null;
  }
  return <BinaryShapingInner meta={meta} threshold={threshold} onChange={onChange} />;
}

function BinaryShapingInner({ meta, threshold, onChange }: Props) {
  // Local mirror lets the slider stay smooth even when the parent re-renders
  // asynchronously; we still drive parent state on every change.
  const [value, setValue] = useState<number>(threshold);

  useEffect(() => {
    setValue(threshold);
  }, [threshold]);

  const handleChange = (next: number) => {
    setValue(next);
    onChange(next);
  };

  return (
    <div className={styles.wrap}>
      <div className={styles.labelRow}>
        <span className={styles.fieldLabel}>Threshold</span>
        <span className={styles.helpRule}>
          {meta.binaryLabels!.on} ≥ CC <Numeric as="span" className={styles.helpRuleAccent}>{value}</Numeric>
        </span>
      </div>
      <Slider
        value={value}
        min={CC_MIN_THRESHOLD}
        max={CC_MAX_THRESHOLD}
        step={1}
        defaultValue={CC_DEFAULT_THRESHOLD}
        onChange={handleChange}
        fillSide="right"
        ariaLabel="Threshold"
      />
    </div>
  );
}
