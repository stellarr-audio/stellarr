import { useStore } from '../../store';
import styles from './LoudnessHistory.module.css';

interface Props {
  blockId: string;
}

const TOTAL_WIDTH = 248;
const TOTAL_HEIGHT = 100;
const LABEL_WIDTH = 26;
const PLOT_WIDTH = TOTAL_WIDTH - LABEL_WIDTH;
const PLOT_HEIGHT = TOTAL_HEIGHT;
const LUFS_MIN = -60;
const LUFS_MAX = 0;

const Y_LABELS = [0, -18, -30, -60];

function lufsToY(lufs: number): number {
  const clamped = Math.max(LUFS_MIN, Math.min(LUFS_MAX, lufs));
  return PLOT_HEIGHT - ((clamped - LUFS_MIN) / (LUFS_MAX - LUFS_MIN)) * PLOT_HEIGHT;
}

export function LoudnessHistory({ blockId }: Props) {
  const history = useStore((s) => s.loudnessHistory);
  const targetLufs = useStore((s) => s.targetLufsByBlockId[blockId]);

  const points = history
    .map((lufs, i) => {
      const x = LABEL_WIDTH + (i / Math.max(history.length - 1, 1)) * PLOT_WIDTH;
      return `${x},${lufsToY(lufs)}`;
    })
    .join(' ');

  const average =
    history.length > 0 ? history.reduce((sum, v) => sum + v, 0) / history.length : null;
  const avgY = average != null ? lufsToY(average) : null;
  const targetY = targetLufs != null ? lufsToY(targetLufs) : null;

  return (
    <svg className={styles.history} width={TOTAL_WIDTH} height={TOTAL_HEIGHT}>
      {/* Y-axis grid lines */}
      {Y_LABELS.map((lufs) => {
        const y = lufsToY(lufs);
        return (
          <g key={lufs}>
            <line x1={LABEL_WIDTH} x2={TOTAL_WIDTH} y1={y} y2={y} className={styles.gridLine} />
            <text x={LABEL_WIDTH - 4} y={y + 3} className={styles.label} textAnchor="end">
              {lufs}
            </text>
          </g>
        );
      })}

      {/* Average line */}
      {avgY != null && (
        <line x1={LABEL_WIDTH} x2={TOTAL_WIDTH} y1={avgY} y2={avgY} className={styles.average} />
      )}

      {/* Target line */}
      {targetY != null && (
        <line
          x1={LABEL_WIDTH}
          x2={TOTAL_WIDTH}
          y1={targetY}
          y2={targetY}
          className={styles.target}
        />
      )}

      {/* History polyline */}
      {history.length > 0 && <polyline points={points} className={styles.line} />}
    </svg>
  );
}
