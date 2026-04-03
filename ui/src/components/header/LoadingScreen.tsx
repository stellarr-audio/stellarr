import { useStore } from '../../store';
import { Logo } from './Logo';
import styles from './LoadingScreen.module.css';

export function LoadingScreen() {
  const loadingStatus = useStore((s) => s.loadingStatus);
  const loadingProgress = useStore((s) => s.loadingProgress);

  return (
    <div className={styles.container}>
      <Logo size={48} className={styles.logo} />

      <span className={styles.title}>Stellarr</span>

      {/* Progress bar */}
      <div className={styles.progressTrack}>
        <div className={styles.progressFill} style={{ width: `${loadingProgress}%` }} />
      </div>

      <span className={styles.status}>{loadingStatus}</span>
    </div>
  );
}
