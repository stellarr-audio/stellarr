import { PiShootingStar } from 'react-icons/pi';
import { useStore } from '../../store';
import {
  requestCheckForUpdates,
  requestInstallUpdate,
  requestOpenReleaseNotes,
} from '../../bridge';
import { Button } from '../common/Button';
import { Row } from './Row';
import styles from './SoftwareUpdates.module.css';

function formatSize(bytes: number): string {
  if (bytes <= 0) return '';
  const mb = bytes / (1024 * 1024);
  return `${mb.toFixed(1)} MB`;
}

function formatReleased(iso: string): string {
  if (!iso) return '';
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return '';
  const days = Math.floor((Date.now() - d.getTime()) / 86_400_000);
  if (days <= 0) return 'Released today';
  if (days === 1) return 'Released yesterday';
  return `Released ${days} days ago`;
}

export function SoftwareUpdates() {
  const state = useStore((s) => s.softwareUpdate);
  const { status } = state;

  const canInstall = status === 'available' || status === 'ready';
  const checkLabel =
    status === 'checking' ? 'Checking…' :
    status === 'downloading' ? 'Downloading…' :
    'Check for updates';
  const installLabel =
    status === 'downloading' ? `Downloading… ${Math.round(state.downloadProgress * 100)}%` :
    status === 'ready' ? 'Restart & install' :
    'Install update';

  return (
    <>
      {(status === 'available' || status === 'downloading' || status === 'ready') && (
        <div className={styles.banner}>
          <PiShootingStar className={styles.bannerIcon} aria-hidden="true" />
          <div className={styles.bannerBody}>
            <div className={styles.bannerTitle}>
              {status === 'ready' ? 'Update ready' : 'Update available'}
              <span className={styles.bannerVersion}>v{state.latestVersion}</span>
            </div>
            <div className={styles.bannerMeta}>
              {formatReleased(state.releasedAt)}
              {state.sizeBytes > 0 && ` · ${formatSize(state.sizeBytes)}`}
            </div>
          </div>
          {state.releaseNotesUrl && (
            <a
              className={styles.bannerLink}
              href={state.releaseNotesUrl}
              onClick={(e) => {
                e.preventDefault();
                requestOpenReleaseNotes(state.releaseNotesUrl);
              }}
            >
              View release notes →
            </a>
          )}
        </div>
      )}

      <Row
        info={
          <p className={styles.blurb}>
            Stellarr looks for new versions on launch and notifies you here — but never installs
            anything on its own. Updates are always a deliberate action, so a pre-show launch
            won't catch you off-guard mid-set.
          </p>
        }
        actions={
          <>
            <Button
              onClick={requestCheckForUpdates}
              disabled={status === 'checking' || status === 'downloading'}
            >
              {checkLabel}
            </Button>
            <Button
              onClick={requestInstallUpdate}
              disabled={!canInstall}
            >
              {installLabel}
            </Button>
          </>
        }
      />

      {status === 'no-update' && (
        <div className={styles.status}>
          <span className={styles.statusDot} aria-hidden="true" />
          You're on the latest version (v{__APP_VERSION__}).
        </div>
      )}

      {status === 'error' && (
        <div className={`${styles.status} ${styles.error}`}>
          {state.error || 'Update check failed.'}
        </div>
      )}
    </>
  );
}
