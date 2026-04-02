import { ChevronLeftIcon, ChevronRightIcon, PlusIcon, UploadIcon, DownloadIcon } from '@radix-ui/react-icons';
import { useStore } from '../store';
import {
  requestNewSession,
  requestSaveSession,
  requestLoadSession,
  requestLoadPresetByIndex,
} from '../bridge';
import { colors } from './colors';

const hoverBg = '#2a2545';

function hoverHandlers() {
  return {
    onMouseEnter: (e: React.MouseEvent<HTMLButtonElement>) => {
      e.currentTarget.style.background = hoverBg;
    },
    onMouseLeave: (e: React.MouseEvent<HTMLButtonElement>) => {
      e.currentTarget.style.background = 'transparent';
    },
  };
}

const iconBtnStyle: React.CSSProperties = {
  background: 'transparent',
  border: `1px solid ${colors.border}`,
  color: colors.muted,
  padding: '0.3rem',
  cursor: 'pointer',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  transition: 'background 0.1s ease',
};

export function PresetBrowser() {
  const presetFiles = useStore((s) => s.presetFiles);
  const currentPresetIndex = useStore((s) => s.currentPresetIndex);

  const currentName =
    currentPresetIndex >= 0 && currentPresetIndex < presetFiles.length
      ? presetFiles[currentPresetIndex].replace('.stellarr', '')
      : 'Untitled';

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
      {/* Preset input group: prev | name | next */}
      <div
        style={{
          display: 'flex',
          alignItems: 'stretch',
          border: `1px solid ${colors.border}`,
        }}
      >
        {/* Prev */}
        <button
          onClick={() => canPrev && requestLoadPresetByIndex(currentPresetIndex - 1)}
          title="Previous preset"
          {...hoverHandlers()}
          style={{
            background: 'transparent',
            border: 'none',
            borderRight: `1px solid ${colors.border}`,
            color: colors.muted,
            padding: '0.3rem 0.4rem',
            cursor: canPrev ? 'pointer' : 'default',
            opacity: canPrev ? 1 : 0.3,
            display: 'flex',
            alignItems: 'center',
            transition: 'background 0.1s ease',
          }}
        >
          <ChevronLeftIcon width={16} height={16} />
        </button>

        {/* Preset name */}
        <span
          style={{
            fontSize: '1rem',
            fontWeight: 600,
            color: currentPresetIndex >= 0 ? colors.text : colors.muted,
            letterSpacing: '0.05em',
            minWidth: 100,
            textAlign: 'center',
            overflow: 'hidden',
            textOverflow: 'ellipsis',
            whiteSpace: 'nowrap',
            maxWidth: 250,
            padding: '0.3rem 1rem',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
          }}
        >
          {currentName}
        </span>

        {/* Next */}
        <button
          onClick={() => canNext && requestLoadPresetByIndex(currentPresetIndex + 1)}
          title="Next preset"
          {...hoverHandlers()}
          style={{
            background: 'transparent',
            border: 'none',
            borderLeft: `1px solid ${colors.border}`,
            color: colors.muted,
            padding: '0.3rem 0.4rem',
            cursor: canNext ? 'pointer' : 'default',
            opacity: canNext ? 1 : 0.3,
            display: 'flex',
            alignItems: 'center',
            transition: 'background 0.1s ease',
          }}
        >
          <ChevronRightIcon width={16} height={16} />
        </button>
      </div>

      {/* New */}
      <button
        onClick={requestNewSession}
        title="New preset"
        {...hoverHandlers()}
        style={iconBtnStyle}
      >
        <PlusIcon width={16} height={16} />
      </button>

      {/* Open */}
      <button
        onClick={requestLoadSession}
        title="Open preset"
        {...hoverHandlers()}
        style={iconBtnStyle}
      >
        <UploadIcon width={16} height={16} />
      </button>

      {/* Save */}
      <button
        onClick={requestSaveSession}
        title="Save preset"
        {...hoverHandlers()}
        style={iconBtnStyle}
      >
        <DownloadIcon width={16} height={16} />
      </button>
    </div>
  );
}
