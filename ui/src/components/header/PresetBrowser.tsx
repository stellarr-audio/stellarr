import { DropdownMenu } from 'radix-ui';
import {
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
  requestAddScene,
  requestRecallScene,
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

const menuItemStyle: React.CSSProperties = {
  padding: '0.35rem 0.75rem',
  fontSize: '1rem',
  color: colors.text,
  cursor: 'pointer',
  outline: 'none',
};

function menuItemHover(e: React.MouseEvent<HTMLDivElement>) {
  e.currentTarget.style.background = colors.border;
}
function menuItemUnhover(e: React.MouseEvent<HTMLDivElement>) {
  e.currentTarget.style.background = 'transparent';
}

export function PresetBrowser() {
  const presetFiles = useStore((s) => s.presetFiles);
  const currentPresetIndex = useStore((s) => s.currentPresetIndex);
  const justSaved = useStore((s) => s.justSaved);
  const scenes = useStore((s) => s.scenes);
  const activeSceneIndex = useStore((s) => s.activeSceneIndex);

  const currentName =
    currentPresetIndex >= 0 && currentPresetIndex < presetFiles.length
      ? presetFiles[currentPresetIndex].replace('.stellarr', '')
      : 'Untitled';

  const currentSceneName =
    activeSceneIndex >= 0 && activeSceneIndex < scenes.length
      ? scenes[activeSceneIndex].name
      : 'No Scene';

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

      {/* Preset dropdown */}
      <PresetDropdown
        label="Preset"
        currentName={currentName}
        presetFiles={presetFiles}
        currentPresetIndex={currentPresetIndex}
      />

      {/* Scene dropdown */}
      <SceneDropdown
        label="Scene"
        currentName={currentSceneName}
        scenes={scenes}
        activeSceneIndex={activeSceneIndex}
      />

      {/* Save split button */}
      <div
        style={{
          display: 'flex',
          alignItems: 'stretch',
          border: `1px solid ${colors.border}`,
        }}
      >
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
          {justSaved ? (
            <CheckIcon width={16} height={16} />
          ) : (
            <BookmarkIcon width={16} height={16} />
          )}
        </button>

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
                style={menuItemStyle}
                onMouseEnter={menuItemHover}
                onMouseLeave={menuItemUnhover}
              >
                Save
              </DropdownMenu.Item>
              <DropdownMenu.Item
                onSelect={requestSaveSession}
                style={menuItemStyle}
                onMouseEnter={menuItemHover}
                onMouseLeave={menuItemUnhover}
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

function PresetDropdown({
  label,
  currentName,
  presetFiles,
  currentPresetIndex,
}: {
  label: string;
  currentName: string;
  presetFiles: string[];
  currentPresetIndex: number;
}) {
  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger asChild>
        <button
          style={{
            display: 'flex',
            alignItems: 'stretch',
            background: 'transparent',
            border: `1px solid ${colors.border}`,
            padding: 0,
            cursor: 'pointer',
            outline: 'none',
          }}
        >
          <span
            style={{
              fontSize: '0.85rem',
              fontWeight: 600,
              color: colors.muted,
              letterSpacing: '0.06em',
              textTransform: 'uppercase',
              background: `${colors.border}88`,
              padding: '0.3rem 0.5rem',
              display: 'flex',
              alignItems: 'center',
            }}
          >
            {label}
          </span>
          <span
            style={{
              fontSize: '0.85rem',
              fontWeight: 600,
              color: currentPresetIndex >= 0 ? colors.text : colors.muted,
              maxWidth: 180,
              overflow: 'hidden',
              textOverflow: 'ellipsis',
              whiteSpace: 'nowrap',
              padding: '0.3rem 0.5rem',
              display: 'flex',
              alignItems: 'center',
              gap: '0.3rem',
            }}
          >
            {currentName}
            <ChevronDownIcon width={12} height={12} color={colors.muted} />
          </span>
        </button>
      </DropdownMenu.Trigger>
      <DropdownMenu.Portal>
        <DropdownMenu.Content
          sideOffset={4}
          style={{
            background: '#1a1535',
            border: `1px solid ${colors.border}`,
            padding: '0.25rem 0',
            minWidth: 160,
            maxHeight: 300,
            overflowY: 'auto',
            zIndex: 20,
          }}
        >
          {presetFiles.length === 0 ? (
            <div
              style={{
                padding: '0.35rem 0.75rem',
                fontSize: '1rem',
                color: colors.muted,
                fontStyle: 'italic',
              }}
            >
              No presets
            </div>
          ) : (
            presetFiles.map((file, i) => (
              <DropdownMenu.Item
                key={i}
                onSelect={() => requestLoadPresetByIndex(i)}
                style={{
                  ...menuItemStyle,
                  fontWeight: i === currentPresetIndex ? 700 : 400,
                  color: i === currentPresetIndex ? colors.primary : colors.text,
                }}
                onMouseEnter={menuItemHover}
                onMouseLeave={menuItemUnhover}
              >
                {file.replace('.stellarr', '')}
              </DropdownMenu.Item>
            ))
          )}
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}

function SceneDropdown({
  label,
  currentName,
  scenes,
  activeSceneIndex,
}: {
  label: string;
  currentName: string;
  scenes: { name: string }[];
  activeSceneIndex: number;
}) {
  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger asChild>
        <button
          style={{
            display: 'flex',
            alignItems: 'stretch',
            background: 'transparent',
            border: `1px solid ${colors.border}`,
            padding: 0,
            cursor: 'pointer',
            outline: 'none',
          }}
        >
          <span
            style={{
              fontSize: '0.85rem',
              fontWeight: 600,
              color: colors.muted,
              letterSpacing: '0.06em',
              textTransform: 'uppercase',
              background: `${colors.border}88`,
              padding: '0.3rem 0.5rem',
              display: 'flex',
              alignItems: 'center',
            }}
          >
            {label}
          </span>
          <span
            style={{
              fontSize: '0.85rem',
              fontWeight: 600,
              color: activeSceneIndex >= 0 ? colors.text : colors.muted,
              maxWidth: 150,
              overflow: 'hidden',
              textOverflow: 'ellipsis',
              whiteSpace: 'nowrap',
              padding: '0.3rem 0.5rem',
              display: 'flex',
              alignItems: 'center',
              gap: '0.3rem',
            }}
          >
            {currentName}
            <ChevronDownIcon width={12} height={12} color={colors.muted} />
          </span>
        </button>
      </DropdownMenu.Trigger>
      <DropdownMenu.Portal>
        <DropdownMenu.Content
          sideOffset={4}
          style={{
            background: '#1a1535',
            border: `1px solid ${colors.border}`,
            padding: '0.25rem 0',
            minWidth: 140,
            zIndex: 20,
          }}
        >
          {scenes.map((scene, i) => (
            <DropdownMenu.Item
              key={i}
              onSelect={() => requestRecallScene(i)}
              style={{
                ...menuItemStyle,
                fontWeight: i === activeSceneIndex ? 700 : 400,
                color: i === activeSceneIndex ? colors.primary : colors.text,
              }}
              onMouseEnter={menuItemHover}
              onMouseLeave={menuItemUnhover}
            >
              {scene.name}
            </DropdownMenu.Item>
          ))}

          {scenes.length > 0 && (
            <DropdownMenu.Separator
              style={{ height: 1, background: colors.border, margin: '0.25rem 0' }}
            />
          )}

          {scenes.length < 16 && (
            <DropdownMenu.Item
              onSelect={requestAddScene}
              style={{ ...menuItemStyle, color: colors.muted }}
              onMouseEnter={menuItemHover}
              onMouseLeave={menuItemUnhover}
            >
              + Add Scene
            </DropdownMenu.Item>
          )}
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}
