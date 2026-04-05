import { useStore } from '../../store';
import styles from './SystemStats.module.css';

function cpuBarColor(percent: number): string {
  if (percent < 40) return 'var(--color-green)';
  if (percent < 70) return '#ffcc00';
  return '#ff4444';
}

function CpuBar() {
  const cpu = useStore((s) => s.cpuPercent);
  const clamped = Math.min(100, Math.max(0, cpu));
  const color = cpuBarColor(clamped);

  return (
    <div className={styles.statBar}>
      <span className={styles.label}>CPU</span>
      <div className={styles.track}>
        <div className={styles.fill} style={{ width: `${clamped}%`, background: color }} />
      </div>
      <span className={styles.value} style={{ color }}>
        {clamped.toFixed(0)}%
      </span>
    </div>
  );
}

const DB_MIN = -60;
const DB_MAX = 6;

function levelToPercent(db: number): number {
  const clamped = Math.max(DB_MIN, Math.min(DB_MAX, db));
  return ((clamped - DB_MIN) / (DB_MAX - DB_MIN)) * 100;
}

function levelBarColor(db: number, clipping: boolean): string {
  if (clipping || db > 0) return '#ff4444';
  if (db > -6) return '#ffcc00';
  return 'var(--color-green)';
}

function OutputLevelBar() {
  const db = useStore((s) => s.outputLevelDb);
  const clipping = useStore((s) => s.outputClipping);
  const percent = levelToPercent(db);
  const color = levelBarColor(db, clipping);
  const display = db <= DB_MIN ? '-inf' : db.toFixed(1);

  return (
    <div className={styles.statBar}>
      <span className={styles.label}>OUT</span>
      <div className={styles.track}>
        <div className={styles.fill} style={{ width: `${percent}%`, background: color }} />
      </div>
      <span className={`${styles.value} ${clipping ? styles.clipping : ''}`} style={{ color }}>
        {display}
      </span>
    </div>
  );
}

export function SystemStats() {
  return (
    <div className={styles.container}>
      <CpuBar />
      <OutputLevelBar />
    </div>
  );
}
