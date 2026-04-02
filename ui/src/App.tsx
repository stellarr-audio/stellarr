import { Tabs } from 'radix-ui';
import { useStore } from './store';
import { Grid } from './grid/Grid';
import { OptionsPanel } from './grid/OptionsPanel';
import { Settings } from './grid/Settings';
import { LoadingScreen } from './grid/LoadingScreen';
import { PresetBrowser } from './grid/PresetBrowser';
import { SystemStats } from './grid/SystemStats';
import { colors } from './grid/colors';
import { Logo } from './grid/Logo';

const tabStyle = (active: boolean): React.CSSProperties => ({
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
  outline: 'none',
});

function App() {
  const loading = useStore((s) => s.loading);
  const selectBlock = useStore((s) => s.selectBlock);
  const showSettings = useStore((s) => s.showSettings);
  const setShowSettings = useStore((s) => s.setShowSettings);

  const activeTab = showSettings ? 'settings' : 'grid';

  if (loading) return <LoadingScreen />;

  return (
    <Tabs.Root
      value={activeTab}
      onValueChange={(v) => setShowSettings(v === 'settings')}
      style={{
        background: colors.bg,
        color: colors.text,
        fontFamily: "'Switzer', system-ui, -apple-system, sans-serif",
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

          <Tabs.List
            style={{
              display: 'flex',
              alignItems: 'center',
              marginLeft: '0.25rem',
            }}
          >
            <Tabs.Trigger value="grid" style={tabStyle(activeTab === 'grid')}>
              Grid
            </Tabs.Trigger>
            <Tabs.Trigger value="settings" style={tabStyle(activeTab === 'settings')}>
              Settings
            </Tabs.Trigger>
          </Tabs.List>
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
        <Tabs.Content
          value="settings"
          forceMount
          hidden={activeTab !== 'settings'}
          style={{ flex: 1, display: activeTab === 'settings' ? 'flex' : 'none' }}
        >
          <Settings />
        </Tabs.Content>

        <Tabs.Content
          value="grid"
          forceMount
          hidden={activeTab !== 'grid'}
          style={{ flex: 1, display: activeTab === 'grid' ? 'flex' : 'none' }}
        >
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
        </Tabs.Content>
      </div>
    </Tabs.Root>
  );
}

export default App;
