import { DropdownMenu } from 'radix-ui';
import {
  ChevronLeftIcon,
  ChevronRightIcon,
  PlusIcon,
  CheckIcon,
  UploadIcon,
  BookmarkIcon,
  ChevronDownIcon,
} from '@radix-ui/react-icons';
import { useStore } from '../../store';
import {
  requestNewSession,
  requestSaveSession,
  requestSaveSessionQuiet,
  requestLoadSession,
  requestLoadPresetByIndex,
} from '../../bridge';
import { colors } from '../common/colors';

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
  const justSaved = useStore((s) => s.justSaved);

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
          onClick={() =>
            canPrev && requestLoadPresetByIndex(currentPresetIndex - 1)
          }
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
          onClick={() =>
            canNext && requestLoadPresetByIndex(currentPresetIndex + 1)
          }
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

      {/* Save split button */}
      <div
        style={{
          display: 'flex',
          alignItems: 'stretch',
          border: `1px solid ${colors.border}`,
        }}
      >
        {/* Save main */}
        <button
          onClick={requestSaveSessionQuiet}
          title="Save preset"
          {...hoverHandlers()}
          style={{
            background: 'transparent',
            border: 'none',
            color: justSaved ? colors.green : colors.muted,
            padding: '0.3rem 0.4rem',
            cursor: 'pointer',
            display: 'flex',
            alignItems: 'center',
            transition: 'color 0.2s ease, background 0.1s ease',
          }}
        >
          {justSaved
            ? <CheckIcon width={16} height={16} />
            : <BookmarkIcon width={16} height={16} />}
        </button>

        {/* Save dropdown */}
        <DropdownMenu.Root>
          <DropdownMenu.Trigger asChild>
            <button
              title="Save options"
              {...hoverHandlers()}
              style={{
                background: 'transparent',
                border: 'none',
                borderLeft: `1px solid ${colors.border}`,
                color: colors.muted,
                padding: '0.3rem 0.2rem',
                cursor: 'pointer',
                display: 'flex',
                alignItems: 'center',
                transition: 'background 0.1s ease',
                outline: 'none',
              }}
            >
              <ChevronDownIcon width={12} height={12} />
            </button>
          </DropdownMenu.Trigger>

          <DropdownMenu.Portal>
            <DropdownMenu.Content
              sideOffset={4}
              align="end"
              style={{
                background: '#1a1535',
                border: `1px solid ${colors.border}`,
                padding: '0.25rem 0',
                minWidth: 100,
                zIndex: 20,
              }}
            >
              <DropdownMenu.Item
                onSelect={requestSaveSessionQuiet}
                style={{
                  padding: '0.35rem 0.75rem',
                  fontSize: '1rem',
                  color: colors.text,
                  cursor: 'pointer',
                  outline: 'none',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = colors.border;
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'transparent';
                }}
              >
                Save
              </DropdownMenu.Item>
              <DropdownMenu.Item
                onSelect={requestSaveSession}
                style={{
                  padding: '0.35rem 0.75rem',
                  fontSize: '1rem',
                  color: colors.text,
                  cursor: 'pointer',
                  outline: 'none',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = colors.border;
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'transparent';
                }}
              >
                Save As...
              </DropdownMenu.Item>
            </DropdownMenu.Content>
          </DropdownMenu.Portal>
        </DropdownMenu.Root>
      </div>
    </div>
  );
}
