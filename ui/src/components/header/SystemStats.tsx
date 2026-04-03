import { useStore } from '../../store';
import styles from './SystemStats.module.css';

function barColor(percent: number): string {
  if (percent < 40) return 'var(--color-green)';
  if (percent < 70) return '#ffcc00';
  return '#ff4444';
}

function StatBar({ label, percent }: { label: string; percent: number }) {
  const clamped = Math.min(100, Math.max(0, percent));
  const color = barColor(clamped);

  return (
    <div className={styles.statBar}>
      <span className={styles.label}>{label}</span>
      <div className={styles.track}>
        <div className={styles.fill} style={{ width: `${clamped}%`, background: color }} />
      </div>
      <span className={styles.percent} style={{ color }}>
        {clamped.toFixed(0)}%
      </span>
    </div>
  );
}

export function SystemStats() {
  const cpu = useStore((s) => s.cpuPercent);
  const memory = useStore((s) => s.memoryMB);
  const totalMemory = useStore((s) => s.totalMemoryMB);

  const memPercent = totalMemory > 0 ? (memory / totalMemory) * 100 : 0;

  return (
    <div className={styles.container}>
      <StatBar label="CPU" percent={cpu} />
      <StatBar label="MEM" percent={memPercent} />
    </div>
  );
}
