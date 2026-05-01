import { useState, useEffect } from 'react';
import { Input } from './Input';
import { MappingPreview } from './shaping/MappingPreview';
import { CurvePreview } from './shaping/CurvePreview';
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

export function ContinuousShaping({ meta, state, onChange }: Props) {
  if (meta.kind !== 'continuous' || !meta.paramRange || !meta.paramFormat || !meta.paramParse) {
    return null;
  }
  const range = meta.paramRange;
  const fmt = meta.paramFormat;
  const parse = meta.paramParse;

  // Local string state for the param inputs (so users can type freely)
  const [paramMinStr, setParamMinStr] = useState(
    fmt(state.paramMin ?? range.min),
  );
  const [paramMaxStr, setParamMaxStr] = useState(
    fmt(state.paramMax ?? range.max),
  );

  useEffect(() => { setParamMinStr(fmt(state.paramMin ?? range.min)); }, [state.paramMin, fmt, range.min]);
  useEffect(() => { setParamMaxStr(fmt(state.paramMax ?? range.max)); }, [state.paramMax, fmt, range.max]);

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
  const commitParamMin = (s: string) => {
    const n = parse(s);
    if (n != null) onChange({ ...state, paramMin: n });
    else setParamMinStr(fmt(state.paramMin ?? range.min));
  };
  const commitParamMax = (s: string) => {
    const n = parse(s);
    if (n != null) onChange({ ...state, paramMax: n });
    else setParamMaxStr(fmt(state.paramMax ?? range.max));
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
          value={state.ccMin}
          onChange={(e) => onChange({ ...state, ccMin: parseInt(e.target.value, 10) || 0 })}
          onBlur={(e) => commitCcMin(parseInt(e.target.value, 10) || 0)}
        />
        <span className={styles.arrow}>→</span>
        <Input
          type="text"
          value={paramMinStr}
          onChange={(e) => setParamMinStr(e.target.value)}
          onBlur={(e) => commitParamMin(e.target.value)}
        />
      </div>
      <div className={styles.grid}>
        <span className={styles.label}>Max</span>
        <Input
          type="number"
          min={1}
          max={127}
          value={state.ccMax}
          onChange={(e) => onChange({ ...state, ccMax: parseInt(e.target.value, 10) || 127 })}
          onBlur={(e) => commitCcMax(parseInt(e.target.value, 10) || 127)}
        />
        <span className={styles.arrow}>→</span>
        <Input
          type="text"
          value={paramMaxStr}
          onChange={(e) => setParamMaxStr(e.target.value)}
          onBlur={(e) => commitParamMax(e.target.value)}
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

      <div className={styles.curveRow}>
        <div className={styles.fg}>
          <span className={styles.fieldLabel}>Curve</span>
          <select
            className={styles.select}
            aria-label="Curve"
            value={state.curve}
            onChange={(e) => onChange({ ...state, curve: e.target.value as MidiCurve })}
          >
            <option value="linear">Linear</option>
            <option value="log">Log</option>
            <option value="exp">Exp</option>
            <option value="sigmoid">S-curve</option>
          </select>
        </div>
        <div className={styles.fg}>
          <span className={styles.fieldLabel}>&nbsp;</span>
          <CurvePreview curve={state.curve} />
        </div>
      </div>
    </>
  );
}
