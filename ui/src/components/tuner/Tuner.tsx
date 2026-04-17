import { useStore } from '../../store';
import { colors } from '../common/colors';
import { StrobeBand } from './StrobeBand';
import styles from './Tuner.module.css';

export function Tuner() {
  const note = useStore((s) => s.tunerNote);
  const octave = useStore((s) => s.tunerOctave);
  const cents = useStore((s) => s.tunerCents);
  const frequency = useStore((s) => s.tunerFrequency);
  const confidence = useStore((s) => s.tunerConfidence);
  const tunerMode = useStore((s) => s.tunerMode);

  const hasSignal = confidence > 0.3 && note !== null;

  const centsColor =
    Math.abs(cents) < 5 ? colors.green : Math.abs(cents) < 15 ? colors.secondary : colors.primary;

  return (
    <div className={styles.container}>
      {/* Note + Octave */}
      <div className={styles.noteRow}>
        <span
          className={`${styles.note} ${!hasSignal ? styles.noteInactive : ''}`}
          style={hasSignal ? { color: centsColor } : undefined}
        >
          {hasSignal ? note : '--'}
        </span>
        <span
          className={`${styles.octave} ${hasSignal ? styles.octaveActive : styles.octaveInactive}`}
        >
          {hasSignal ? octave : ''}
        </span>
      </div>

      {/* Frequency */}
      <span
        className={`${styles.frequency} ${hasSignal ? styles.frequencyActive : styles.frequencyInactive}`}
      >
        {hasSignal ? `${frequency.toFixed(1)} Hz` : '-- Hz'}
      </span>

      {/* Tuner visualisation */}
      {tunerMode === 'strobe' ? (
        <StrobeBand />
      ) : (
        <div className={styles.centsBar}>
          <div className={styles.track}>
            <div className={styles.centerMark} />
            {hasSignal && (
              <div
                className={styles.needle}
                style={{
                  left: `${50 + cents}%`,
                  background: centsColor,
                  boxShadow: `0 0 12px ${centsColor}88`,
                }}
              />
            )}
          </div>
          <div className={styles.labels}>
            <span>-50</span>
            <span>-25</span>
            <span>0</span>
            <span>+25</span>
            <span>+50</span>
          </div>
        </div>
      )}
    </div>
  );
}
