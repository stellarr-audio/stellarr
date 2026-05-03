import { useEffect, useState } from 'react';
import type { TargetMeta } from './shaping/targetMeta';
import styles from './BinaryShaping.module.css';

interface Props {
  meta: TargetMeta;
  threshold: number;
  onChange: (next: number) => void;
}

const CC_MIN_THRESHOLD = 1;
const CC_MAX_THRESHOLD = 127;

const clamp = (v: number, lo: number, hi: number) => Math.max(lo, Math.min(hi, v));

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

  const handleChange = (raw: string) => {
    const n = parseInt(raw, 10);
    if (!Number.isFinite(n)) return;
    const clamped = clamp(n, CC_MIN_THRESHOLD, CC_MAX_THRESHOLD);
    setValue(clamped);
    onChange(clamped);
  };

  const pct =
    ((value - CC_MIN_THRESHOLD) / (CC_MAX_THRESHOLD - CC_MIN_THRESHOLD)) * 100;
  // Clamp the floating "CC N" label so it doesn't bleed past the track edges
  // at extremes (translateX(-50%) would push it past the parent at 0%/100%).
  const labelPct = clamp(pct, 8, 92);

  return (
    <div className={styles.wrap}>
      <div className={styles.labelRow}>
        <span className={styles.fieldLabel}>Threshold</span>
        <span className={styles.helpRule}>
          {meta.binaryLabels!.on} when <span className={styles.helpRuleAccent}>CC ≥ {value}</span>
        </span>
      </div>
      <div className={styles.sliderShell}>
        <div className={styles.trackOnFill} style={{ left: `${pct}%`, right: '0' }} />
        <input
          type="range"
          min={CC_MIN_THRESHOLD}
          max={CC_MAX_THRESHOLD}
          step={1}
          value={value}
          onChange={(e) => handleChange(e.target.value)}
          className={styles.slider}
          aria-label="Threshold"
        />
        <div className={styles.handleLabel} style={{ left: `${labelPct}%` }}>
          CC {value}
        </div>
      </div>
    </div>
  );
}
