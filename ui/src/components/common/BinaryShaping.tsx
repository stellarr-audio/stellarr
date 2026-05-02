import { useEffect, useState } from 'react';
import { Input } from './Input';
import { InputGroup, InputGroupLabel } from './InputGroup';
import type { TargetMeta } from './shaping/targetMeta';
import styles from './BinaryShaping.module.css';

interface Props {
  meta: TargetMeta;
  threshold: number;
  onChange: (next: number) => void;
}

// CC values span 0..127 = 128 discrete buckets.
const CC_BUCKETS = 128;
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
  const [str, setStr] = useState<string>(String(threshold));

  useEffect(() => {
    const parsed = parseInt(str, 10);
    if (!Number.isFinite(parsed) || parsed !== threshold) {
      setStr(String(threshold));
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [threshold]);

  const commit = (raw: string) => {
    const n = parseInt(raw, 10);
    if (Number.isFinite(n)) {
      onChange(clamp(n, CC_MIN_THRESHOLD, CC_MAX_THRESHOLD));
    }
  };

  const handleBlur = () => {
    const n = parseInt(str, 10);
    if (
      !Number.isFinite(n) ||
      n < CC_MIN_THRESHOLD ||
      n > CC_MAX_THRESHOLD ||
      n !== threshold
    ) {
      setStr(String(threshold));
    }
  };

  // Visual split: threshold sits at the boundary. CC < threshold = OFF, CC ≥ threshold = ON.
  // Width of the OFF region = threshold / CC_BUCKETS.
  const splitPct = (threshold / CC_BUCKETS) * 100;
  // Clamp the marker label's anchor so "CC 1" / "CC 127" stay within the bar
  // edges. The marker line itself stays at the exact boundary.
  const labelLeftPct = Math.max(8, Math.min(92, splitPct));

  return (
    <div className={styles.wrap}>
      <div className={styles.row}>
        <InputGroup>
          <InputGroupLabel className={styles.prefix}>Threshold</InputGroupLabel>
          <Input
            inGroup
            type="number"
            min={CC_MIN_THRESHOLD}
            max={CC_MAX_THRESHOLD}
            step={1}
            value={str}
            onChange={(e) => {
              setStr(e.target.value);
              commit(e.target.value);
            }}
            onBlur={handleBlur}
          />
        </InputGroup>
      </div>

      <div className={styles.barWrap}>
        <div className={styles.bar}>
          <div className={styles.off} style={{ width: `${splitPct}%` }}>
            {meta.binaryLabels!.off}
          </div>
          <div className={styles.on} style={{ width: `${100 - splitPct}%` }}>
            {meta.binaryLabels!.on}
          </div>
          <div className={styles.marker} style={{ left: `${splitPct}%` }} />
        </div>
        <div className={styles.markerLabel} style={{ left: `${labelLeftPct}%` }}>
          CC {threshold}
        </div>
      </div>

      <p className={styles.helpText}>{meta.binaryHelpTemplate!(threshold)}</p>
    </div>
  );
}
