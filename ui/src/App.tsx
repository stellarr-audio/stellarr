import { useEffect } from 'react';
import { useStore } from './store';
import { useSyncTheme } from './hooks/useSyncTheme';
import { Grid } from './components/grid/Grid';
import { GridOverlay } from './components/grid/GridOverlay';
import { GridResizer } from './components/grid/GridResizer';
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
import { Tablist, Tab } from './components/common/Tablist';
import { TbLayoutGrid, TbWaveSine, TbPlug, TbSunHigh, TbMoon, TbSettings } from 'react-icons/tb';
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

  const panelClass = (id: string) =>
    `${styles.tabContent} ${activeTab === id ? styles.tabContentVisible : ''}`;

  return (
    <div className={styles.root}>
      {/* Header */}
      <div className={styles.header}>
        <div className={styles.headerLeft}>
          <div className={styles.brand}>
            <Logo size={22} className={styles.brandLogo} />
            <span className={styles.brandName}>Stellarr</span>
          </div>

          <Tablist
            value={activeTab}
            onChange={handleTabChange}
            aria-label="Main navigation"
            className={styles.headerTablist}
          >
            <Tooltip content="Grid" side="bottom">
              <Tab id="grid" title="Grid">
                <TbLayoutGrid size={20} />
              </Tab>
            </Tooltip>
            <Tooltip content="Tuner" side="bottom">
              <Tab id="tuner" title="Tuner">
                <TbWaveSine size={20} />
              </Tab>
            </Tooltip>
            <Tooltip content="MIDI" side="bottom">
              <Tab id="midi" title="MIDI">
                <TbPlug size={20} />
              </Tab>
            </Tooltip>
            <Tooltip content="System" side="bottom">
              <Tab id="settings" title="System">
                <TbSettings size={20} />
              </Tab>
            </Tooltip>
          </Tablist>
        </div>

        {/* Centre: preset browser */}
        <div className={styles.headerCenter}>
          <PresetBrowser />
        </div>

        {/* Right: theme toggle */}
        <ThemeToggle />
      </div>

      {/* Main area — all panels stay mounted; visibility toggled by activeTab */}
      <div className={styles.main}>
        <div role="tabpanel" hidden={activeTab !== 'grid'} className={panelClass('grid')}>
          <div
            onClick={(e) => {
              const t = e.target as HTMLElement;
              if (t.closest('[data-grid-block]') || t.closest('[data-options-panel]')) return;
              selectBlock(null);
            }}
            className={styles.gridArea}
          >
            <GridOverlay />
            <GridResizer>
              <Grid />
            </GridResizer>
          </div>
          <OptionsPanel />
        </div>

        <div role="tabpanel" hidden={activeTab !== 'tuner'} className={panelClass('tuner')}>
          <Tuner />
          <TunerPanel />
        </div>

        <div role="tabpanel" hidden={activeTab !== 'midi'} className={panelClass('midi')}>
          <MidiPage />
          <MidiMonitor />
        </div>

        <div role="tabpanel" hidden={activeTab !== 'settings'} className={panelClass('settings')}>
          <Settings />
        </div>
      </div>

      {/* Footer: CPU / IN / OUT meters */}
      <Footer />
    </div>
  );
}

function ThemeToggle() {
  const theme = useThemeStore((s) => s.theme);
  const setTheme = useThemeStore((s) => s.setTheme);
  const resolved = resolveTheme(theme);

  const flip = () => setTheme(resolved === 'dark' ? 'light' : 'dark');

  const icon = resolved === 'dark' ? <TbMoon size={18} /> : <TbSunHigh size={18} />;
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
