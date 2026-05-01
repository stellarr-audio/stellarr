import type { MidiCurve } from '../../../store';
import styles from './CurvePreview.module.css';

interface Props {
  curve: MidiCurve;
  inGroup?: boolean;
}

const W = 50;
const H = 22;
const SAMPLES = 24;

function curvePath(curve: MidiCurve): string {
  const points: Array<[number, number]> = [];
  for (let i = 0; i <= SAMPLES; i++) {
    const t = i / SAMPLES;
    let y: number;
    switch (curve) {
      case 'linear':  y = t; break;
      case 'log':     y = Math.log10(1 + 9 * t); break;
      case 'exp':     y = (Math.pow(10, t) - 1) / 9; break;
      case 'sigmoid': {
        const k = 5;
        const top = Math.tanh((t - 0.5) * k);
        const bot = Math.tanh(0.5 * k);
        y = 0.5 + 0.5 * top / bot;
        break;
      }
    }
    points.push([t * W, H - y * H]);
  }
  return points.map(([x, y], i) => `${i === 0 ? 'M' : 'L'}${x},${y}`).join(' ');
}

export function CurvePreview({ curve, inGroup }: Props) {
  return (
    <div className={`${styles.preview} ${inGroup ? styles.inGroup : ''}`}>
      <svg viewBox={`0 0 ${W} ${H}`} width={W} height={H}>
        <path className={styles.path} d={curvePath(curve)} />
      </svg>
    </div>
  );
}
