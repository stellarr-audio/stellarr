import { useStore } from './store';
import { Grid } from './grid/Grid';
import { OptionsPanel } from './grid/OptionsPanel';
import { Settings } from './grid/Settings';
import { LoadingScreen } from './grid/LoadingScreen';
import { PresetBrowser } from './grid/PresetBrowser';
import { SystemStats } from './grid/SystemStats';
import { colors } from './grid/colors';
import { Logo } from './grid/Logo';

type Tab = 'grid' | 'settings';

function TabButton({
  label,
  active,
  onClick,
}: {
  label: string;
  active: boolean;
  onClick: () => void;
}) {
  return (
    <button
      onClick={onClick}
      style={{
        background: active ? colors.border : 'transparent',
        border: 'none',
        borderBottom: active ? `2px solid ${colors.primary}` : '2px solid transparent',
        color: active ? colors.text : colors.muted,
        padding: '0.6rem 1rem',
        fontSize: '1rem',
        fontWeight: 600,
        letterSpacing: '0.08em',
        textTransform: 'uppercase',
        cursor: 'pointer',
      }}
    >
      {label}
    </button>
  );
}

function App() {
  const loading = useStore((s) => s.loading);
  const selectBlock = useStore((s) => s.selectBlock);
  const showSettings = useStore((s) => s.showSettings);
  const setShowSettings = useStore((s) => s.setShowSettings);

  const activeTab: Tab = showSettings ? 'settings' : 'grid';

  const setTab = (tab: Tab) => setShowSettings(tab === 'settings');

  if (loading) return <LoadingScreen />;

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
          padding: '0.5rem 1.25rem',
          borderBottom: `1px solid ${colors.border}`,
          flexShrink: 0,
        }}
      >
        <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
          <div
            style={{
              display: 'flex',
              alignItems: 'center',
              gap: '0.5rem',
              padding: '0.5rem 0',
            }}
          >
            <Logo
              size={22}
              color={colors.primary}
              style={{ filter: `drop-shadow(0 0 4px ${colors.primary}88)` }}
            />
            <span
              style={{
                fontSize: '1rem',
                fontWeight: 700,
                color: colors.primary,
                letterSpacing: '0.08em',
                textTransform: 'uppercase',
                textShadow: `0 0 12px ${colors.primary}44`,
              }}
            >
              Stellarr
            </span>
          </div>

          <div
            style={{
              display: 'flex',
              alignItems: 'center',
              marginLeft: '0.25rem',
            }}
          >
            <TabButton
              label="Grid"
              active={activeTab === 'grid'}
              onClick={() => setTab('grid')}
            />
            <TabButton
              label="Settings"
              active={activeTab === 'settings'}
              onClick={() => setTab('settings')}
            />
          </div>
        </div>

        {/* Centre: preset browser */}
        <div style={{ flex: 1, display: 'flex', justifyContent: 'center' }}>
          <PresetBrowser />
        </div>

        {/* Right: system stats */}
        <SystemStats />
      </div>

      {/* Main area */}
      <div style={{ flex: 1, display: 'flex', overflow: 'hidden' }}>
        {activeTab === 'settings' ? (
          <Settings />
        ) : (
          <>
            <div
              onClick={(e) => {
                if (e.target === e.currentTarget) selectBlock(null);
              }}
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
            <OptionsPanel />
          </>
        )}
      </div>
    </div>
  );
}

export default App;
