import { line as d3Line } from 'd3-shape';
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

// SVG layout (viewBox units == px when not scaled)
const VB_W = 320;
const VB_H = 80;
const PAD_LEFT = 26;    // y-axis label gutter
const PAD_RIGHT = 12;
const PAD_TOP = 6;
const PAD_BOTTOM = 16;  // x-axis tick label gutter
const PLOT_W = VB_W - PAD_LEFT - PAD_RIGHT;
const PLOT_H = VB_H - PAD_TOP - PAD_BOTTOM;

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
  // Map a CC value to plot-X
  const xOf = (cc: number) => PAD_LEFT + (cc / 127) * PLOT_W;
  // Map a param value to plot-Y (inverted: top of plot = paramRange.max)
  const yOf = (p: number) => {
    const t = (p - paramRange.min) / (paramRange.max - paramRange.min);
    return PAD_TOP + (1 - t) * PLOT_H;
  };

  // Curve segment points
  const SAMPLES = 48;
  const points: Array<[number, number]> = [];
  for (let i = 0; i <= SAMPLES; i++) {
    const tCc = i / SAMPLES;
    const cc = ccMin + tCc * (ccMax - ccMin);
    const t = curveT(tCc, curve);
    const param = paramMin + t * (paramMax - paramMin);
    points.push([xOf(cc), yOf(param)]);
  }
  const lineGen = d3Line<[number, number]>().x((d) => d[0]).y((d) => d[1]);
  const segmentD = lineGen(points) ?? '';

  // Flat clamp lines outside [ccMin, ccMax]
  const leftClampD = lineGen([
    [PAD_LEFT, yOf(paramMin)],
    [xOf(ccMin), yOf(paramMin)],
  ]) ?? '';
  const rightClampD = lineGen([
    [xOf(ccMax), yOf(paramMax)],
    [PAD_LEFT + PLOT_W, yOf(paramMax)],
  ]) ?? '';

  return (
    <div className={styles.preview}>
      <svg
        viewBox={`0 0 ${VB_W} ${VB_H}`}
        preserveAspectRatio="xMidYMid meet"
        className={styles.svg}
      >
        {/* Plot border */}
        <rect
          x={PAD_LEFT}
          y={PAD_TOP}
          width={PLOT_W}
          height={PLOT_H}
          className={styles.plotBox}
        />

        {/* Clamp segments (dashed) */}
        <path className={styles.dashed} d={leftClampD} />
        <path className={styles.dashed} d={rightClampD} />

        {/* Curve segment (solid) */}
        <path className={styles.curve} d={segmentD} />

        {/* Anchors */}
        <circle cx={xOf(ccMin)} cy={yOf(paramMin)} r={3} className={styles.anchor} />
        <circle cx={xOf(ccMax)} cy={yOf(paramMax)} r={3} className={styles.anchor} />

        {/* X-axis tick labels */}
        <text
          x={PAD_LEFT}
          y={VB_H - 3}
          textAnchor="middle"
          className={styles.tick}
        >0</text>
        <text
          x={xOf(ccMin)}
          y={VB_H - 3}
          textAnchor="middle"
          className={styles.tick}
        >{ccMin}</text>
        <text
          x={xOf(ccMax)}
          y={VB_H - 3}
          textAnchor="middle"
          className={styles.tick}
        >{ccMax}</text>
        <text
          x={PAD_LEFT + PLOT_W}
          y={VB_H - 3}
          textAnchor="middle"
          className={styles.tick}
        >127</text>

        {/* Y-axis labels */}
        <text
          x={PAD_LEFT - 3}
          y={PAD_TOP + 3}
          textAnchor="end"
          dominantBaseline="hanging"
          className={styles.axisLabel}
        >{paramRange.max.toString()}</text>
        <text
          x={PAD_LEFT - 3}
          y={PAD_TOP + PLOT_H}
          textAnchor="end"
          dominantBaseline="alphabetic"
          className={styles.axisLabel}
        >{paramRange.min.toString()}</text>
      </svg>
    </div>
  );
}
