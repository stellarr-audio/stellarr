import { useEffect } from 'react';
import { Tabs } from 'radix-ui';
import { useStore } from './store';
import { useSyncTheme } from './hooks/useSyncTheme';
import { Grid } from './components/grid/Grid';
import { GridOverlay } from './components/grid/GridOverlay';
import { OptionsPanel } from './components/options/OptionsPanel';
import { Settings } from './components/settings/Settings';
import { Tuner } from './components/tuner/Tuner';
import { TunerPanel } from './components/tuner/TunerPanel';
import { MidiPage } from './components/midi/MidiPage';
import { MidiMonitor } from './components/midi/MidiMonitor';
import { LoadingScreen } from './components/header/LoadingScreen';
import { PresetBrowser } from './components/header/PresetBrowser';
import { Logo } from './components/header/Logo';
import { Footer } from './components/footer/Footer';
import { Tooltip } from './components/common/Tooltip';
import { IconButton } from './components/common/IconButton';
import { TbLayoutGrid, TbWaveSine, TbPlug, TbSun, TbMoon } from 'react-icons/tb';
import { LuSettings } from 'react-icons/lu';
import { useThemeStore, resolveTheme } from './store/theme';
import { requestSetTunerEnabled, requestSaveSessionQuiet } from './bridge';
import styles from './App.module.css';

function App() {
  useSyncTheme();
  const loading = useStore((s) => s.loading);
  const selectBlock = useStore((s) => s.selectBlock);
  const activeTab = useStore((s) => s.activeTab);
  const setActiveTab = useStore((s) => s.setActiveTab);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 's') {
        e.preventDefault();
        requestSaveSessionQuiet();
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  if (loading) return <LoadingScreen />;

  const handleTabChange = (tab: string) => {
    setActiveTab(tab);
    requestSetTunerEnabled(tab === 'tuner');
  };

  return (
    <Tabs.Root value={activeTab} onValueChange={handleTabChange} className={styles.root}>
      {/* Header */}
      <div className={styles.header}>
        <div className={styles.headerLeft}>
          <div className={styles.brand}>
            <Logo size={22} className={styles.brandLogo} />
            <span className={styles.brandName}>Stellarr</span>
          </div>

          <Tabs.List className={styles.tabList}>
            <Tooltip content="Grid" side="bottom">
              <Tabs.Trigger
                value="grid"
                className={`${styles.tab} ${activeTab === 'grid' ? styles.tabActive : ''}`}
              >
                <TbLayoutGrid size={20} />
              </Tabs.Trigger>
            </Tooltip>
            <Tooltip content="Tuner" side="bottom">
              <Tabs.Trigger
                value="tuner"
                className={`${styles.tab} ${activeTab === 'tuner' ? styles.tabActive : ''}`}
              >
                <TbWaveSine size={20} />
              </Tabs.Trigger>
            </Tooltip>
            <Tooltip content="MIDI" side="bottom">
              <Tabs.Trigger
                value="midi"
                className={`${styles.tab} ${activeTab === 'midi' ? styles.tabActive : ''}`}
              >
                <TbPlug size={20} />
              </Tabs.Trigger>
            </Tooltip>
            <Tooltip content="System" side="bottom">
              <Tabs.Trigger
                value="settings"
                className={`${styles.tab} ${activeTab === 'settings' ? styles.tabActive : ''}`}
              >
                <LuSettings size={20} />
              </Tabs.Trigger>
            </Tooltip>
          </Tabs.List>
        </div>

        {/* Centre: preset browser */}
        <div className={styles.headerCenter}>
          <PresetBrowser />
        </div>

        {/* Right: theme toggle */}
        <ThemeToggle />
      </div>

      {/* Main area */}
      <div className={styles.main}>
        <Tabs.Content
          value="grid"
          forceMount
          hidden={activeTab !== 'grid'}
          className={`${styles.tabContent} ${activeTab === 'grid' ? styles.tabContentVisible : ''}`}
        >
          <div
            onClick={(e) => {
              if (e.target === e.currentTarget) selectBlock(null);
            }}
            className={styles.gridArea}
          >
            <GridOverlay />
            <Grid />
          </div>
          <OptionsPanel />
        </Tabs.Content>

        <Tabs.Content
          value="tuner"
          forceMount
          hidden={activeTab !== 'tuner'}
          className={`${styles.tabContent} ${activeTab === 'tuner' ? styles.tabContentVisible : ''}`}
        >
          <Tuner />
          <TunerPanel />
        </Tabs.Content>

        <Tabs.Content
          value="midi"
          forceMount
          hidden={activeTab !== 'midi'}
          className={`${styles.tabContent} ${activeTab === 'midi' ? styles.tabContentVisible : ''}`}
        >
          <MidiPage />
          <MidiMonitor />
        </Tabs.Content>

        <Tabs.Content
          value="settings"
          forceMount
          hidden={activeTab !== 'settings'}
          className={`${styles.tabContent} ${activeTab === 'settings' ? styles.tabContentVisible : ''}`}
        >
          <Settings />
        </Tabs.Content>
      </div>

      {/* Footer: CPU / IN / OUT meters */}
      <Footer />
    </Tabs.Root>
  );
}

function ThemeToggle() {
  const theme = useThemeStore((s) => s.theme);
  const setTheme = useThemeStore((s) => s.setTheme);
  const resolved = resolveTheme(theme);

  const flip = () => setTheme(resolved === 'dark' ? 'light' : 'dark');

  const icon = resolved === 'dark' ? <TbMoon size={18} /> : <TbSun size={18} />;
  const label = resolved === 'dark' ? 'Switch to light mode' : 'Switch to dark mode';

  return (
    <Tooltip content={label} side="bottom">
      <span>
        <IconButton icon={icon} onClick={flip} title={label} />
      </span>
    </Tooltip>
  );
}

export default App;
