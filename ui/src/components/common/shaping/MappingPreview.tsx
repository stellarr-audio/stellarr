import type { MidiCurve } from '../../../store';
import styles from './MappingPreview.module.css';

interface Props {
  ccMin: number;
  ccMax: number;
  paramMin: number;
  paramMax: number;
  paramRange: { min: number; max: number };
  curve: MidiCurve;
}

const VB_W = 280;
const VB_H = 48;

function curveT(t: number, curve: MidiCurve): number {
  switch (curve) {
    case 'linear':  return t;
    case 'log':     return Math.log10(1 + 9 * t);
    case 'exp':     return (Math.pow(10, t) - 1) / 9;
    case 'sigmoid': {
      const k = 5;
      const top = Math.tanh((t - 0.5) * k);
      const bot = Math.tanh(0.5 * k);
      return 0.5 + 0.5 * top / bot;
    }
  }
}

export function MappingPreview({
  ccMin, ccMax, paramMin, paramMax, paramRange, curve,
}: Props) {
  const xOf = (cc: number) => (cc / 127) * VB_W;
  const yNorm = (p: number) => (p - paramRange.min) / (paramRange.max - paramRange.min);
  const yOf = (p: number) => VB_H - yNorm(p) * VB_H;

  const SAMPLES = 32;
  const points: string[] = [];
  for (let i = 0; i <= SAMPLES; i++) {
    const tCc = i / SAMPLES;
    const cc = ccMin + tCc * (ccMax - ccMin);
    const t = curveT(tCc, curve);
    const param = paramMin + t * (paramMax - paramMin);
    points.push(`${i === 0 ? 'M' : 'L'}${xOf(cc).toFixed(2)},${yOf(param).toFixed(2)}`);
  }
  const segmentD = points.join(' ');

  const leftClampD  = `M0,${yOf(paramMin).toFixed(2)} L${xOf(ccMin).toFixed(2)},${yOf(paramMin).toFixed(2)}`;
  const rightClampD = `M${xOf(ccMax).toFixed(2)},${yOf(paramMax).toFixed(2)} L${VB_W},${yOf(paramMax).toFixed(2)}`;

  const aLeftPct  = (xOf(ccMin) / VB_W) * 100;
  const aRightPct = (xOf(ccMax) / VB_W) * 100;
  const aLeftBottomPct = yNorm(paramMin) * 100;
  const aRightBottomPct = yNorm(paramMax) * 100;

  return (
    <div className={styles.preview}>
      <svg className={styles.svg} viewBox={`0 0 ${VB_W} ${VB_H}`} preserveAspectRatio="none">
        <path className={styles.dashed} d={leftClampD} />
        <path className={styles.curve} d={segmentD} />
        <path className={styles.dashed} d={rightClampD} />
      </svg>
      <div className={styles.anchor} style={{ left: `${aLeftPct}%`, bottom: `${aLeftBottomPct}%` }} />
      <div className={styles.anchor} style={{ left: `${aRightPct}%`, bottom: `${aRightBottomPct}%` }} />

      <span className={styles.tick} style={{ left: 0 }}>CC 0</span>
      <span className={styles.tick} style={{ left: `${aLeftPct}%` }}>{ccMin}</span>
      <span className={styles.tick} style={{ left: `${aRightPct}%` }}>{ccMax}</span>
      <span className={styles.tick} style={{ left: '100%' }}>127</span>

      <span className={styles.axisLabel} style={{ top: 0 }}>
        {paramRange.max.toString()}
      </span>
      <span className={styles.axisLabel} style={{ bottom: 6 }}>
        {paramRange.min.toString()}
      </span>
    </div>
  );
}
