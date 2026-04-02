import { useStore } from '../store';
import {
  requestSaveSession,
  requestLoadSession,
  requestPickPresetDirectory,
  requestLoadPresetByIndex,
} from '../bridge';
import { colors } from './colors';

export function PresetBrowser() {
  const presetDirectory = useStore((s) => s.presetDirectory);
  const presetFiles = useStore((s) => s.presetFiles);
  const currentPresetIndex = useStore((s) => s.currentPresetIndex);

  const dirName = presetDirectory
    ? presetDirectory.split('/').pop() ?? presetDirectory
    : '';

  const currentName =
    currentPresetIndex >= 0 && currentPresetIndex < presetFiles.length
      ? presetFiles[currentPresetIndex].replace('.stellarr', '')
      : 'No preset';

  const canPrev = currentPresetIndex > 0;
  const canNext =
    presetFiles.length > 0 && currentPresetIndex < presetFiles.length - 1;

  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'center',
        gap: '0.4rem',
      }}
    >
      {/* Directory picker */}
      <button
        onClick={requestPickPresetDirectory}
        title={presetDirectory || 'Select preset directory'}
        style={{
          background: 'transparent',
          border: `1px solid ${colors.border}`,
          color: dirName ? colors.text : colors.muted,
          padding: '0.2rem 0.4rem',
          fontSize: '0.55rem',
          fontWeight: 600,
          letterSpacing: '0.05em',
          textTransform: 'uppercase',
          cursor: 'pointer',
          maxWidth: 80,
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          whiteSpace: 'nowrap',
        }}
      >
        {dirName || 'Folder'}
      </button>

      {/* Prev arrow */}
      <button
        onClick={() => canPrev && requestLoadPresetByIndex(currentPresetIndex - 1)}
        style={{
          background: 'transparent',
          border: 'none',
          color: canPrev ? colors.text : colors.muted,
          fontSize: '0.7rem',
          cursor: canPrev ? 'pointer' : 'default',
          padding: '0 0.15rem',
          opacity: canPrev ? 1 : 0.3,
        }}
      >
        ‹
      </button>

      {/* Current preset name */}
      <span
        style={{
          fontSize: '0.6rem',
          fontWeight: 600,
          color: currentPresetIndex >= 0 ? colors.text : colors.muted,
          letterSpacing: '0.05em',
          minWidth: 60,
          textAlign: 'center',
          overflow: 'hidden',
          textOverflow: 'ellipsis',
          whiteSpace: 'nowrap',
          maxWidth: 120,
        }}
      >
        {currentName}
      </span>

      {/* Next arrow */}
      <button
        onClick={() => canNext && requestLoadPresetByIndex(currentPresetIndex + 1)}
        style={{
          background: 'transparent',
          border: 'none',
          color: canNext ? colors.text : colors.muted,
          fontSize: '0.7rem',
          cursor: canNext ? 'pointer' : 'default',
          padding: '0 0.15rem',
          opacity: canNext ? 1 : 0.3,
        }}
      >
        ›
      </button>

      {/* Save button */}
      <button
        onClick={requestSaveSession}
        style={{
          background: 'transparent',
          border: `1px solid ${colors.border}`,
          color: colors.muted,
          padding: '0.2rem 0.4rem',
          fontSize: '0.5rem',
          fontWeight: 600,
          letterSpacing: '0.06em',
          textTransform: 'uppercase',
          cursor: 'pointer',
        }}
      >
        Save
      </button>

      {/* Load button */}
      <button
        onClick={requestLoadSession}
        style={{
          background: 'transparent',
          border: `1px solid ${colors.border}`,
          color: colors.muted,
          padding: '0.2rem 0.4rem',
          fontSize: '0.5rem',
          fontWeight: 600,
          letterSpacing: '0.06em',
          textTransform: 'uppercase',
          cursor: 'pointer',
        }}
      >
        Load
      </button>
    </div>
  );
}
