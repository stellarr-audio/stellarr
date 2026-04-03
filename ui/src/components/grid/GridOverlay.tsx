import { useStore } from '../../store';
import { colors } from '../common/colors';

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
