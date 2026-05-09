import { LuSparkles } from 'react-icons/lu';
import { useStore } from '../../store';
import { GridToolbar } from './GridToolbar';
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

  return (
    <div className={styles.strip}>
      <div className={styles.left}>
        <span className={styles.preset}>{presetName ?? 'Untitled'}</span>
        {sceneName && (
          <>
            <span className={styles.sep} aria-hidden="true">
              <LuSparkles size={16} />
            </span>
            <span className={styles.scene}>{sceneName}</span>
          </>
        )}
      </div>
      <div className={styles.right}>
        <GridToolbar />
      </div>
    </div>
  );
}
