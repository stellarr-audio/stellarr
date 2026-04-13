import { useStore } from '../../store';
import styles from './LoudnessHistory.module.css';

interface Props {
  blockId: string;
}

const HISTORY_WIDTH = 248;
const HISTORY_HEIGHT = 40;
const LUFS_MIN = -60;
const LUFS_MAX = 0;

function lufsToY(lufs: number): number {
  const clamped = Math.max(LUFS_MIN, Math.min(LUFS_MAX, lufs));
  return HISTORY_HEIGHT - ((clamped - LUFS_MIN) / (LUFS_MAX - LUFS_MIN)) * HISTORY_HEIGHT;
}

export function LoudnessHistory({ blockId }: Props) {
  const history = useStore((s) => s.loudnessHistory);
  const targetLufs = useStore((s) => s.targetLufsByBlockId[blockId]);

  if (history.length === 0) {
    return (
      <div className={styles.history} style={{ width: HISTORY_WIDTH, height: HISTORY_HEIGHT }} />
    );
  }

  const points = history
    .map((lufs, i) => {
      const x = (i / Math.max(history.length - 1, 1)) * HISTORY_WIDTH;
      return `${x},${lufsToY(lufs)}`;
    })
    .join(' ');

  const average = history.reduce((sum, v) => sum + v, 0) / history.length;
  const avgY = lufsToY(average);
  const targetY = targetLufs != null ? lufsToY(targetLufs) : null;

  return (
    <svg className={styles.history} width={HISTORY_WIDTH} height={HISTORY_HEIGHT}>
      <polyline points={points} className={styles.line} />
      <line x1={0} x2={HISTORY_WIDTH} y1={avgY} y2={avgY} className={styles.average} />
      {targetY != null && (
        <line x1={0} x2={HISTORY_WIDTH} y1={targetY} y2={targetY} className={styles.target} />
      )}
    </svg>
  );
}
