import { useStore } from '../../store';
import { colors } from '../common/colors';

export function Tuner() {
  const note = useStore((s) => s.tunerNote);
  const octave = useStore((s) => s.tunerOctave);
  const cents = useStore((s) => s.tunerCents);
  const frequency = useStore((s) => s.tunerFrequency);
  const confidence = useStore((s) => s.tunerConfidence);

  const hasSignal = confidence > 0.3 && note !== null;

  const centsColor =
    Math.abs(cents) < 5 ? colors.green : Math.abs(cents) < 15 ? '#ffaa00' : colors.primary;

  return (
    <div
      style={{
        flex: 1,
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        gap: '2.5rem',
        background: colors.bg,
      }}
    >
      {/* Note + Octave */}
      <div style={{ display: 'flex', alignItems: 'baseline', gap: '0.5rem' }}>
        <span
          style={{
            fontSize: '12rem',
            fontWeight: 700,
            color: hasSignal ? centsColor : `${colors.muted}22`,
            letterSpacing: '0.04em',
            lineHeight: 1,
            transition: 'color 0.2s ease',
          }}
        >
          {hasSignal ? note : '--'}
        </span>
        <span
          style={{
            fontSize: '4rem',
            fontWeight: 600,
            color: hasSignal ? colors.muted : `${colors.muted}22`,
            lineHeight: 1,
          }}
        >
          {hasSignal ? octave : ''}
        </span>
      </div>

      {/* Frequency */}
      <span
        style={{
          fontSize: '1.8rem',
          color: hasSignal ? colors.muted : `${colors.muted}22`,
          fontVariantNumeric: 'tabular-nums',
          letterSpacing: '0.04em',
        }}
      >
        {hasSignal ? `${frequency.toFixed(1)} Hz` : '-- Hz'}
      </span>

      {/* Cents bar */}
      <div
        style={{
          width: 600,
          maxWidth: '80%',
          position: 'relative',
        }}
      >
        {/* Track */}
        <div
          style={{
            width: '100%',
            height: 6,
            background: colors.border,
            position: 'relative',
          }}
        >
          {/* Center mark */}
          <div
            style={{
              position: 'absolute',
              left: '50%',
              top: -10,
              width: 3,
              height: 26,
              background: colors.muted,
              transform: 'translateX(-50%)',
            }}
          />

          {/* Needle */}
          {hasSignal && (
            <div
              style={{
                position: 'absolute',
                left: `${50 + cents}%`,
                top: -12,
                width: 6,
                height: 30,
                background: centsColor,
                transform: 'translateX(-50%)',
                transition: 'left 0.05s ease, background 0.2s ease',
                boxShadow: `0 0 12px ${centsColor}88`,
              }}
            />
          )}
        </div>

        {/* Labels */}
        <div
          style={{
            display: 'flex',
            justifyContent: 'space-between',
            marginTop: '1rem',
            fontSize: '1rem',
            color: colors.muted,
            letterSpacing: '0.06em',
          }}
        >
          <span>-50</span>
          <span>-25</span>
          <span>0</span>
          <span>+25</span>
          <span>+50</span>
        </div>
      </div>
    </div>
  );
}
