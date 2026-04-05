import { useStore } from '../../store';
import {
  requestScanPlugins,
  requestPickScanDirectory,
  requestRemoveScanDirectory,
} from '../../bridge';
import styles from './Settings.module.css';

export function Settings() {
  const scanDirectories = useStore((s) => s.scanDirectories);
  const availablePlugins = useStore((s) => s.availablePlugins);
  const scanning = useStore((s) => s.scanning);

  return (
    <div className={styles.container}>
      <div className={styles.content}>
        {/* Libraries section */}
        <Section title="Libraries">
          <div className={styles.dirList}>
            {scanDirectories.map((dir) => (
              <div key={dir.path} className={styles.dirRow}>
                <div className={styles.dirInfo}>
                  <span className={styles.dirPath}>{dir.path}</span>
                  {dir.isDefault && <span className={styles.dirBadge}>System</span>}
                </div>
                {!dir.isDefault && (
                  <button
                    onClick={() => requestRemoveScanDirectory(dir.path)}
                    className={styles.removeBtn}
                  >
                    x
                  </button>
                )}
              </div>
            ))}
          </div>

          <div className={styles.actions}>
            <button className={styles.actionBtn} onClick={requestPickScanDirectory}>
              Add Directory
            </button>
            <button className={styles.actionBtn} onClick={requestScanPlugins} disabled={scanning}>
              {scanning ? 'Scanning...' : 'Scan Now'}
            </button>
          </div>
        </Section>

        {/* Discovered plugins section */}
        <Section title={`Discovered Plugins (${availablePlugins.length})`}>
          {availablePlugins.length === 0 ? (
            <div className={styles.emptyText}>
              No plugins found. Add library directories and click "Scan Now".
            </div>
          ) : (
            <div className={styles.pluginList}>
              {availablePlugins.map((plugin) => (
                <div key={plugin.id} className={styles.pluginRow}>
                  <div>
                    <div className={styles.pluginName}>{plugin.name}</div>
                    <div className={styles.pluginManufacturer}>{plugin.manufacturer}</div>
                  </div>
                  <span className={styles.pluginFormat}>{plugin.format}</span>
                </div>
              ))}
            </div>
          )}
        </Section>
      </div>

      {/* App info panel */}
      <div className={styles.infoPanel}>
        <span className={styles.infoTitle}>Stellarr</span>
        <span className={styles.infoVersion}>v0.1.0</span>
        <p className={styles.infoDescription}>
          Made by an AI and a human, together. Free and open, forever. For the love of music and
          art.
        </p>
      </div>
    </div>
  );
}

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div className={styles.section}>
      <h3 className={styles.sectionTitle}>{title}</h3>
      {children}
    </div>
  );
}
