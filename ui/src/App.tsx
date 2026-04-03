import { Tabs } from 'radix-ui';
import { useStore } from './store';
import { Grid } from './components/grid/Grid';
import { OptionsPanel } from './components/options/OptionsPanel';
import { Settings } from './components/settings/Settings';
import { LoadingScreen } from './components/header/LoadingScreen';
import { PresetBrowser } from './components/header/PresetBrowser';
import { SystemStats } from './components/header/SystemStats';
import { colors } from './components/common/colors';
import { Logo } from './components/header/Logo';

function GridOverlay() {
  const presetFiles = useStore((s) => s.presetFiles);
  const currentPresetIndex = useStore((s) => s.currentPresetIndex);
  const scenes = useStore((s) => s.scenes);
  const activeSceneIndex = useStore((s) => s.activeSceneIndex);

  const presetName =
    currentPresetIndex >= 0 && currentPresetIndex < presetFiles.length
      ? presetFiles[currentPresetIndex].replace('.stellarr', '')
      : null;

  const sceneName =
    activeSceneIndex >= 0 && activeSceneIndex < scenes.length
      ? scenes[activeSceneIndex].name
      : null;

  if (!presetName && !sceneName) return null;

  return (
    <div
      style={{
        width: '100%',
        maxWidth: 1040,
        textAlign: 'center',
        padding: '0.5rem 0',
        borderBottom: `1px solid ${colors.border}`,
        flexShrink: 0,
      }}
    >
      <span
        style={{
          fontSize: '1.8rem',
          fontWeight: 700,
          letterSpacing: '0.08em',
          textTransform: 'uppercase',
          color: `${colors.primary}`,
        }}
      >
        {presetName ?? 'Untitled'}
        {sceneName && (
          <>
            <span style={{ margin: '0 0.6rem' }}>✦</span>
            {sceneName}
          </>
        )}
      </span>
    </div>
  );
}

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
            <Logo size={22} style={{ filter: `drop-shadow(0 0 4px ${colors.primary}88)` }} />
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
              flexDirection: 'column',
              alignItems: 'center',
              justifyContent: 'center',
              padding: '1rem',
              gap: '1rem',
            }}
          >
            <GridOverlay />
            <Grid />
          </div>
          <OptionsPanel />
        </Tabs.Content>
      </div>
    </Tabs.Root>
  );
}

export default App;
