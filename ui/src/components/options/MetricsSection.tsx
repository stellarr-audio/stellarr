import type { GridBlock } from '../../store';
import styles from './MetricsSection.module.css';

interface Props {
  block: GridBlock;
}

export function MetricsSection({ block }: Props) {
  const peakDb = block.peakDb ?? -60;

  // Normalise dB to 0-100% for the meter (-60dB = 0%, 0dB = 100%)
  const meterPercent = Math.max(0, Math.min(100, ((peakDb + 60) / 60) * 100));
  const isClipping = peakDb > 0;

  const dbDisplay = peakDb <= -60 ? '-inf' : `${peakDb.toFixed(1)}`;

  return (
    <div className={styles.section}>
      <div className={styles.row}>
        <span className={styles.label}>Signal</span>
        <span className={`${styles.value} ${isClipping ? styles.valueClipping : ''}`}>
          {dbDisplay} dB
        </span>
      </div>
      <div className={styles.meterTrack}>
        <div
          className={`${styles.meterFill} ${isClipping ? styles.meterClipping : ''}`}
          style={{ width: `${meterPercent}%` }}
        />
      </div>
    </div>
  );
}
