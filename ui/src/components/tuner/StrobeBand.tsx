import { useEffect, useRef } from 'react';
import { useStore } from '../../store';
import { colors } from '../common/colors';
import styles from './StrobeBand.module.css';

const STRIPE_WIDTH = 28;
const SPEED_FACTOR = 1.8;

function centsColor(cents: number): string {
  const abs = Math.abs(cents);
  if (abs < 5) return colors.green;
  if (abs < 15) return colors.secondary;
  return colors.primary;
}

export function StrobeBand() {
  const cents = useStore((s) => s.tunerCents);
  const confidence = useStore((s) => s.tunerConfidence);
  const note = useStore((s) => s.tunerNote);

  const bandRef = useRef<HTMLDivElement>(null);
  const phaseRef = useRef(0);
  const rafRef = useRef(0);
  const centsRef = useRef(cents);
  const hasSignalRef = useRef(false);

  const hasSignal = confidence > 0.3 && note !== null;
  centsRef.current = cents;
  hasSignalRef.current = hasSignal;

  useEffect(() => {
    const animate = () => {
      if (bandRef.current && hasSignalRef.current) {
        phaseRef.current += centsRef.current * SPEED_FACTOR;
        bandRef.current.style.backgroundPositionX = `${phaseRef.current}px`;
      }
      rafRef.current = requestAnimationFrame(animate);
    };

    rafRef.current = requestAnimationFrame(animate);
    return () => cancelAnimationFrame(rafRef.current);
  }, []);

  const colour = hasSignal ? centsColor(cents) : 'var(--color-muted)';
  const opacity = hasSignal ? 1 : 0.15;

  return (
    <div className={styles.container}>
      <div
        ref={bandRef}
        className={styles.band}
        style={{
          backgroundImage: `repeating-linear-gradient(
            90deg,
            ${colour} 0px,
            ${colour} ${STRIPE_WIDTH / 2}px,
            transparent ${STRIPE_WIDTH / 2}px,
            transparent ${STRIPE_WIDTH}px
          )`,
          opacity,
        }}
      />
      <div className={styles.centerMark} />
    </div>
  );
}
