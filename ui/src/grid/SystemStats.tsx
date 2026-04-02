import { useStore } from '../store';
import { colors } from './colors';

function barColor(percent: number): string {
  if (percent < 40) return colors.green;
  if (percent < 70) return '#ffcc00';
  return '#ff4444';
}

function StatBar({ label, percent }: { label: string; percent: number }) {
  const clamped = Math.min(100, Math.max(0, percent));
  const color = barColor(clamped);

  return (
    <div style={{ display: 'flex', alignItems: 'center', gap: '0.4rem' }}>
      <span
        style={{
          fontSize: '1rem',
          color: colors.muted,
          letterSpacing: '0.06em',
        }}
      >
        {label}
      </span>
      <div
        style={{
          width: 50,
          height: 6,
          background: colors.border,
          borderRadius: 3,
          overflow: 'hidden',
        }}
      >
        <div
          style={{
            width: `${clamped}%`,
            height: '100%',
            background: color,
            borderRadius: 3,
            transition: 'width 0.3s ease, background 0.3s ease',
          }}
        />
      </div>
      <span
        style={{
          fontSize: '1rem',
          color,
          fontWeight: 600,
          width: '3ch',
          textAlign: 'right',
          fontVariantNumeric: 'tabular-nums',
        }}
      >
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
    <div
      style={{
        display: 'flex',
        alignItems: 'center',
        gap: '1rem',
        flexShrink: 0,
      }}
    >
      <StatBar label="CPU" percent={cpu} />
      <StatBar label="MEM" percent={memPercent} />
    </div>
  );
}
