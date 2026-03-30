import { useStore } from './store';
import { Grid } from './grid/Grid';
import { Palette } from './grid/Palette';
import { colors } from './grid/colors';

function App() {
  const connected = useStore((s) => s.connected);

  return (
    <div
      style={{
        background: colors.bg,
        color: colors.text,
        fontFamily: 'system-ui, -apple-system, sans-serif',
        letterSpacing: '0.02em',
        height: '100vh',
        boxSizing: 'border-box',
        overflow: 'hidden',
        display: 'flex',
        flexDirection: 'column',
      }}
    >
      {/* Header */}
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          padding: '0.6rem 1rem',
          borderBottom: `1px solid ${colors.border}`,
          flexShrink: 0,
        }}
      >
        <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
          <span
            style={{
              fontSize: '0.9rem',
              fontWeight: 700,
              color: colors.primary,
              letterSpacing: '0.08em',
              textTransform: 'uppercase',
              textShadow: `0 0 12px ${colors.primary}44`,
            }}
          >
            Stellarr
          </span>
          <Palette />
        </div>

        <div style={{ display: 'flex', alignItems: 'center', gap: '0.4rem' }}>
          <div
            style={{
              width: 6,
              height: 6,
              borderRadius: '50%',
              background: connected ? colors.green : colors.muted,
              boxShadow: connected ? `0 0 6px ${colors.green}` : 'none',
            }}
          />
          <span
            style={{
              color: connected ? colors.green : colors.muted,
              fontSize: '0.6rem',
              letterSpacing: '0.1em',
              textTransform: 'uppercase',
            }}
          >
            {connected ? 'Connected' : 'Waiting'}
          </span>
        </div>
      </div>

      {/* Grid area */}
      <div
        style={{
          flex: 1,
          overflow: 'auto',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          padding: '1rem',
        }}
      >
        <Grid />
      </div>
    </div>
  );
}

export default App;
