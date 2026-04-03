import { useStore } from '../../store';
import { Logo } from './Logo';
import { colors } from '../common/colors';

export function LoadingScreen() {
  const loadingStatus = useStore((s) => s.loadingStatus);
  const loadingProgress = useStore((s) => s.loadingProgress);

  return (
    <div
      style={{
        background: colors.bg,
        color: colors.text,
        
        height: '100vh',
        display: 'flex',
        flexDirection: 'column',
        alignItems: 'center',
        justifyContent: 'center',
        gap: '1.5rem',
      }}
    >
      <Logo
        size={48}
        color={colors.primary}
        style={{ filter: `drop-shadow(0 0 8px ${colors.primary}66)` }}
      />

      <span
        style={{
          fontSize: '1.2rem',
          fontWeight: 700,
          color: colors.primary,
          letterSpacing: '0.12em',
          textTransform: 'uppercase',
          textShadow: `0 0 16px ${colors.primary}44`,
        }}
      >
        Stellarr
      </span>

      {/* Progress bar */}
      <div
        style={{
          width: 280,
          height: 3,
          background: colors.border,
          borderRadius: 2,
          overflow: 'hidden',
        }}
      >
        <div
          style={{
            width: `${loadingProgress}%`,
            height: '100%',
            background: colors.primary,
            transition: 'width 0.3s ease',
          }}
        />
      </div>

      <span
        style={{
          fontSize: '1rem',
          color: colors.muted,
          letterSpacing: '0.08em',
          textTransform: 'uppercase',
        }}
      >
        {loadingStatus}
      </span>
    </div>
  );
}
