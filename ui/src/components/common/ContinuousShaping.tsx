import { useEffect, useState } from 'react';
import { MappingPreview } from './shaping/MappingPreview';
import { CurvePreview } from './shaping/CurvePreview';
import { Input } from './Input';
import { InputGroup, InputGroupLabel } from './InputGroup';
import type { TargetMeta } from './shaping/targetMeta';
import type { MidiCurve } from '../../store';
import styles from './ContinuousShaping.module.css';

export interface ShapingState {
  ccMin: number;
  ccMax: number;
  paramMin: number | undefined; // undefined = use target default
  paramMax: number | undefined;
  curve: MidiCurve;
}

interface Props {
  meta: TargetMeta;
  state: ShapingState;
  onChange: (next: ShapingState) => void;
}

const clamp = (v: number, lo: number, hi: number) => Math.max(lo, Math.min(hi, v));

interface ParamInputProps {
  meta: TargetMeta;
  value: number;            // canonical
  onCommit: (canonical: number) => void;
}

function ParamInput({ meta, value, onCommit }: ParamInputProps) {
  // Local text state so the user can type intermediate forms ("-", "0.", "")
  // without immediate normalisation. Live-commit on every keystroke that
  // parses to a finite number — that's what drives the live graph update.
  const [str, setStr] = useState<string>(String(meta.paramToDisplay!(value)));

  useEffect(() => {
    const expected = meta.paramToDisplay!(value);
    const parsed = parseFloat(str);
    // Only resync from `value` when local text doesn't match — preserves
    // mid-edit text like "-" or "0." while still picking up external changes
    // (e.g. up/down arrow steppers from the input itself).
    if (!Number.isFinite(parsed) || Math.abs(parsed - expected) > 1e-6) {
      setStr(String(expected));
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [value]);

  const handleChange = (raw: string) => {
    setStr(raw);
    const n = parseFloat(raw);
    if (Number.isFinite(n)) {
      const clamped = clamp(n, meta.paramInputMin!, meta.paramInputMax!);
      onCommit(meta.paramFromDisplay!(clamped));
    }
  };

  const handleBlur = () => {
    const expected = meta.paramToDisplay!(value);
    const n = parseFloat(str);
    // Revert when input is non-numeric OR when the typed value disagrees with
    // the committed canonical value (e.g. "200" was clamped to 100 but the
    // commit was a no-op because state was already at 100).
    if (!Number.isFinite(n) || Math.abs(n - expected) > 1e-6) {
      setStr(String(expected));
    }
  };

  const inputEl = (
    <Input
      inGroup={!!meta.paramSuffix}
      type="number"
      min={meta.paramInputMin}
      max={meta.paramInputMax}
      step={meta.paramInputStep}
      value={str}
      onChange={(e) => handleChange(e.target.value)}
      onBlur={handleBlur}
      className={meta.paramSuffix ? undefined : styles.compactInput}
    />
  );

  if (meta.paramSuffix) {
    return (
      <InputGroup className={styles.compactInputGroup}>
        {inputEl}
        <InputGroupLabel>{meta.paramSuffix}</InputGroupLabel>
      </InputGroup>
    );
  }
  return inputEl;
}

export function ContinuousShaping({ meta, state, onChange }: Props) {
  if (
    meta.kind !== 'continuous'
    || !meta.paramRange
    || meta.paramInputMin == null
    || meta.paramInputMax == null
    || meta.paramInputStep == null
    || !meta.paramToDisplay
    || !meta.paramFromDisplay
  ) {
    return null;
  }
  const range = meta.paramRange;

  const commitCcMin = (v: number) => {
    // Cap at 126 so there's always room for ccMax > ccMin. The input itself
    // accepts 0..127; we just guarantee a non-empty CC range in committed state.
    const ccMin = Math.min(126, clamp(Math.round(v), 0, 127));
    const ccMax = state.ccMax > ccMin ? state.ccMax : Math.min(127, ccMin + 1);
    onChange({ ...state, ccMin, ccMax });
  };
  const commitCcMax = (v: number) => {
    // Floor at 1 so there's always room for ccMin < ccMax.
    const ccMax = Math.max(1, clamp(Math.round(v), 0, 127));
    const ccMin = state.ccMin < ccMax ? state.ccMin : Math.max(0, ccMax - 1);
    onChange({ ...state, ccMin, ccMax });
  };

  const previewParamMin = state.paramMin ?? range.min;
  const previewParamMax = state.paramMax ?? range.max;

  return (
    <>
      <div className={styles.grid}>
        <span className={styles.label}></span>
        <span className={styles.colLabel}>CC value</span>
        <span></span>
        <span className={styles.colLabel}>{meta.paramLabel}</span>
      </div>
      <div className={styles.grid}>
        <span className={styles.label}>Min</span>
        <Input
          type="number"
          min={0}
          max={127}
          step={1}
          value={state.ccMin}
          onChange={(e) => {
            const n = parseInt(e.target.value, 10);
            commitCcMin(Number.isNaN(n) ? 0 : n);
          }}
          className={styles.compactInput}
        />
        <span className={styles.arrow}>→</span>
        <ParamInput
          meta={meta}
          value={state.paramMin ?? range.min}
          onCommit={(canonical) => onChange({ ...state, paramMin: canonical })}
        />
      </div>
      <div className={styles.grid}>
        <span className={styles.label}>Max</span>
        <Input
          type="number"
          min={0}
          max={127}
          step={1}
          value={state.ccMax}
          onChange={(e) => {
            const n = parseInt(e.target.value, 10);
            commitCcMax(Number.isNaN(n) ? 127 : n);
          }}
          className={styles.compactInput}
        />
        <span className={styles.arrow}>→</span>
        <ParamInput
          meta={meta}
          value={state.paramMax ?? range.max}
          onCommit={(canonical) => onChange({ ...state, paramMax: canonical })}
        />
      </div>

      <MappingPreview
        ccMin={state.ccMin}
        ccMax={state.ccMax}
        paramMin={previewParamMin}
        paramMax={previewParamMax}
        paramRange={range}
        curve={state.curve}
      />

      <p className={styles.helpText}>
        CC values inside the range map to the parameter range. Outside = clamped. Set Min &gt; Max in either column to invert.
      </p>

      <InputGroup>
        <InputGroupLabel>Curve</InputGroupLabel>
        <select
          className={styles.selectInGroup}
          aria-label="Curve"
          value={state.curve}
          onChange={(e) => onChange({ ...state, curve: e.target.value as MidiCurve })}
        >
          <option value="linear">Linear</option>
          <option value="log">Log</option>
          <option value="exp">Exp</option>
          <option value="sigmoid">S-curve</option>
        </select>
        <CurvePreview curve={state.curve} inGroup />
      </InputGroup>
    </>
  );
}
