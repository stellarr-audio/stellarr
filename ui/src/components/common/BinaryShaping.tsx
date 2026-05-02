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

const clamp = (v: number, lo: number, hi: number) => Math.max(lo, Math.min(hi, v));

export function BinaryShaping({ meta, threshold, onChange }: Props) {
  if (meta.kind !== 'binary' || !meta.binaryLabels || !meta.binaryHelpTemplate) {
    return null;
  }

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
      onChange(clamp(n, 1, 127));
    }
  };

  const handleBlur = () => {
    const n = parseInt(str, 10);
    if (!Number.isFinite(n) || n < 1 || n > 127 || n !== threshold) {
      setStr(String(threshold));
    }
  };

  // Visual split: threshold sits at the boundary. CC < threshold = OFF, CC ≥ threshold = ON.
  // Width of the OFF region = threshold / 128 (since CC range is 0..127, 128 buckets).
  const splitPct = (threshold / 128) * 100;

  return (
    <div className={styles.wrap}>
      <div className={styles.row}>
        <InputGroup>
          <InputGroupLabel className={styles.prefix}>Threshold</InputGroupLabel>
          <Input
            inGroup
            type="number"
            min={1}
            max={127}
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
            {meta.binaryLabels.off}
          </div>
          <div className={styles.on} style={{ width: `${100 - splitPct}%` }}>
            {meta.binaryLabels.on}
          </div>
          <div className={styles.marker} style={{ left: `${splitPct}%` }} />
        </div>
        <div className={styles.markerLabel} style={{ left: `${splitPct}%` }}>
          CC {threshold}
        </div>
      </div>

      <p className={styles.helpText}>{meta.binaryHelpTemplate(threshold)}</p>
    </div>
  );
}
