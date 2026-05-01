import { useState, useEffect } from 'react';
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
  const display = meta.paramToDisplay!(value);
  const [str, setStr] = useState<string>(String(display));

  useEffect(() => { setStr(String(meta.paramToDisplay!(value))); }, [value, meta]);

  const commit = () => {
    const n = parseFloat(str);
    if (Number.isFinite(n)) {
      const clamped = clamp(n, meta.paramInputMin!, meta.paramInputMax!);
      onCommit(meta.paramFromDisplay!(clamped));
    } else {
      // revert to current canonical value
      setStr(String(meta.paramToDisplay!(value)));
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
      onChange={(e) => setStr(e.target.value)}
      onBlur={commit}
    />
  );

  if (meta.paramSuffix) {
    return (
      <InputGroup>
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
    const ccMin = clamp(Math.round(v), 0, 126);
    const ccMax = ccMin >= state.ccMax ? Math.min(127, ccMin + 1) : state.ccMax;
    onChange({ ...state, ccMin, ccMax });
  };
  const commitCcMax = (v: number) => {
    const ccMax = clamp(Math.round(v), 1, 127);
    const ccMin = ccMax <= state.ccMin ? Math.max(0, ccMax - 1) : state.ccMin;
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
          max={126}
          step={1}
          value={state.ccMin}
          onChange={(e) => onChange({ ...state, ccMin: parseInt(e.target.value, 10) || 0 })}
          onBlur={(e) => commitCcMin(parseInt(e.target.value, 10) || 0)}
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
          min={1}
          max={127}
          step={1}
          value={state.ccMax}
          onChange={(e) => onChange({ ...state, ccMax: parseInt(e.target.value, 10) || 127 })}
          onBlur={(e) => commitCcMax(parseInt(e.target.value, 10) || 127)}
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
