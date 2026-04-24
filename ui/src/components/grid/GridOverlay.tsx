import { useStore } from '../../store';
import styles from './GridOverlay.module.css';

export function GridOverlay() {
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
    <div className={styles.strip}>
      <span className={styles.preset}>{presetName ?? 'Untitled'}</span>
      {sceneName && <span className={styles.scenePill}>{sceneName}</span>}
    </div>
  );
}
