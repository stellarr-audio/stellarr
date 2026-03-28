import { useStore } from './store';
import { sendEvent } from './bridge';

const colors = {
  bg: '#0d0b1a',
  primary: '#ff2d7b',
  text: '#e0dff0',
  muted: '#5a5478',
  green: '#00ff9d',
};

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
        padding: '2.5rem 3rem',
        boxSizing: 'border-box',
        overflow: 'hidden',
        display: 'flex',
        flexDirection: 'column',
      }}
    >
      <div>
        <h1
          style={{
            fontSize: '2rem',
            fontWeight: 700,
            color: colors.primary,
            textShadow: `0 0 20px ${colors.primary}66, 0 0 40px ${colors.primary}33`,
            margin: '0 0 0.25rem 0',
            letterSpacing: '0.08em',
            textTransform: 'uppercase',
          }}
        >
          Stellarr
        </h1>
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            gap: '0.5rem',
            margin: '0 0 2.5rem 0',
          }}
        >
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
              fontSize: '0.7rem',
              letterSpacing: '0.12em',
              textTransform: 'uppercase',
            }}
          >
            {connected ? 'Engine Connected' : 'Waiting for Engine'}
          </span>
        </div>
      </div>

      <button
        onClick={() => sendEvent('uiAction', 'buttonClicked')}
        style={{
          alignSelf: 'flex-start',
          background: 'transparent',
          color: colors.primary,
          border: `1px solid ${colors.primary}`,
          padding: '0.6rem 1.6rem',
          fontSize: '0.8rem',
          fontWeight: 600,
          letterSpacing: '0.1em',
          textTransform: 'uppercase',
          cursor: 'pointer',
          transition: 'all 0.2s ease',
          boxShadow: `0 0 12px ${colors.primary}33`,
        }}
        onMouseEnter={(e) => {
          e.currentTarget.style.background = colors.primary;
          e.currentTarget.style.color = colors.bg;
          e.currentTarget.style.boxShadow = `0 0 24px ${colors.primary}66`;
        }}
        onMouseLeave={(e) => {
          e.currentTarget.style.background = 'transparent';
          e.currentTarget.style.color = colors.primary;
          e.currentTarget.style.boxShadow = `0 0 12px ${colors.primary}33`;
        }}
      >
        Ping C++
      </button>
    </div>
  );
}

export default App;
