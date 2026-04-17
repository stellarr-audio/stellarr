import { useMemo } from 'react';
import { TbX } from 'react-icons/tb';
import { useStore } from '../../store';
import {
  requestScanPlugins,
  requestPickScanDirectory,
  requestRemoveScanDirectory,
  requestSetTelemetryEnabled,
} from '../../bridge';
import { Button } from '../common/Button';
import { IconButton } from '../common/IconButton';
import { ToggleSwitch } from '../common/ToggleSwitch';
import styles from './Settings.module.css';

export function Settings() {
  const scanDirectories = useStore((s) => s.scanDirectories);
  const availablePlugins = useStore((s) => s.availablePlugins);
  const sortedPlugins = useMemo(
    () => availablePlugins.toSorted((a, b) => a.name.localeCompare(b.name)),
    [availablePlugins],
  );
  const scanning = useStore((s) => s.scanning);
  const telemetryEnabled = useStore((s) => s.telemetryEnabled);

  return (
    <div className={styles.container}>
      <div className={styles.content}>
        <Section title="Libraries">
          <div className={styles.dirList}>
            {scanDirectories.map((dir) => (
              <div key={dir.path} className={styles.dirRow}>
                <div className={styles.dirInfo}>
                  <span className={styles.dirPath}>{dir.path}</span>
                  {dir.isDefault && <span className={styles.dirBadge}>System</span>}
                </div>
                {!dir.isDefault && (
                  <IconButton
                    size="sm"
                    variant="danger"
                    icon={<TbX size={14} />}
                    title="Remove directory"
                    onClick={() => requestRemoveScanDirectory(dir.path)}
                  />
                )}
              </div>
            ))}
          </div>

          <div className={styles.actions}>
            <Button onClick={requestPickScanDirectory}>Add Directory</Button>
            <Button onClick={requestScanPlugins} disabled={scanning}>
              {scanning ? 'Scanning...' : 'Scan Now'}
            </Button>
          </div>
        </Section>

        <Section title={`Discovered Plugins (${availablePlugins.length})`}>
          {availablePlugins.length === 0 ? (
            <div className={styles.emptyText}>
              No plugins found. Add library directories and click "Scan Now".
            </div>
          ) : (
            <div className={styles.pluginList}>
              {sortedPlugins.map((plugin) => (
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

        <Section title="Privacy">
          <div className={styles.privacyRow}>
            <div className={styles.privacyInfo}>
              <span className={styles.privacyLabel}>Send crash reports</span>
              <span className={styles.privacyDescription}>
                Anonymous crash data only. No plugin names, presets, audio, or personal information.
                Takes effect on next launch.
              </span>
            </div>
            <ToggleSwitch
              enabled={telemetryEnabled}
              onToggle={() => requestSetTelemetryEnabled(!telemetryEnabled)}
              title="Toggle crash reporting"
            />
          </div>
        </Section>
      </div>

      <div className={styles.infoPanel}>
        <span className={styles.infoTitle}>Stellarr</span>
        <span className={styles.infoVersion}>v{__APP_VERSION__}</span>
        <p className={styles.infoDescription}>
          Made by an AI and a human, together. Free and open, forever. For music. For noise. For
          every unheard voice.
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
