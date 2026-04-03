import { useState } from 'react';
import { DropdownMenu } from 'radix-ui';
import {
  CheckIcon,
  UploadIcon,
  BookmarkIcon,
  ChevronDownIcon,
  DotsHorizontalIcon,
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
  requestRenameScene,
  requestDeleteScene,
} from '../../bridge';
import { colors } from '../common/colors';
import { SceneRenameDialog } from './SceneRenameDialog';

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

const menuHover = {
  onMouseEnter: (e: React.MouseEvent<HTMLDivElement>) => {
    e.currentTarget.style.background = colors.border;
  },
  onMouseLeave: (e: React.MouseEvent<HTMLDivElement>) => {
    e.currentTarget.style.background = 'transparent';
  },
};

const dropdownContentStyle: React.CSSProperties = {
  background: '#1a1535',
  border: `1px solid ${colors.border}`,
  padding: '0.25rem 0',
  minWidth: 'var(--radix-dropdown-menu-trigger-width)',
  zIndex: 20,
};

// -- Shared trigger content for preset/scene dropdowns ------------------------

function DropdownTriggerContent({
  label,
  value,
  hasValue,
}: {
  label: string;
  value: string;
  hasValue: boolean;
}) {
  return (
    <>
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
          width: 140,
          padding: '0.3rem 0.5rem',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          gap: '0.3rem',
        }}
      >
        <span
          style={{
            fontSize: '0.85rem',
            fontWeight: 600,
            color: hasValue ? colors.text : colors.muted,
            overflow: 'hidden',
            textOverflow: 'ellipsis',
            whiteSpace: 'nowrap',
          }}
        >
          {value}
        </span>
        <ChevronDownIcon width={12} height={12} color={colors.muted} style={{ flexShrink: 0 }} />
      </span>
    </>
  );
}

// -- Menu item wrapper --------------------------------------------------------

function MenuItem({
  onSelect,
  style,
  children,
}: {
  onSelect: (e: Event) => void;
  style?: React.CSSProperties;
  children: React.ReactNode;
}) {
  return (
    <DropdownMenu.Item onSelect={onSelect} style={{ ...menuItemStyle, ...style }} {...menuHover}>
      {children}
    </DropdownMenu.Item>
  );
}

// -- Main component -----------------------------------------------------------

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
    <div style={{ display: 'flex', alignItems: 'center', gap: '0.4rem' }}>
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
        currentName={currentName}
        presetFiles={presetFiles}
        currentPresetIndex={currentPresetIndex}
      />

      {/* Scene dropdown */}
      <SceneDropdown
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
              style={{ ...dropdownContentStyle, minWidth: 100 }}
            >
              <MenuItem onSelect={requestSaveSessionQuiet}>Save</MenuItem>
              <MenuItem onSelect={requestSaveSession}>Save As...</MenuItem>
            </DropdownMenu.Content>
          </DropdownMenu.Portal>
        </DropdownMenu.Root>
      </div>
    </div>
  );
}

// -- Preset dropdown ----------------------------------------------------------

function PresetDropdown({
  currentName,
  presetFiles,
  currentPresetIndex,
}: {
  currentName: string;
  presetFiles: string[];
  currentPresetIndex: number;
}) {
  return (
    <DropdownMenu.Root>
      <DropdownMenu.Trigger
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
        <DropdownTriggerContent
          label="Preset"
          value={currentName}
          hasValue={currentPresetIndex >= 0}
        />
      </DropdownMenu.Trigger>
      <DropdownMenu.Portal>
        <DropdownMenu.Content
          sideOffset={4}
          style={{ ...dropdownContentStyle, maxHeight: 300, overflowY: 'auto' }}
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
              <MenuItem
                key={i}
                onSelect={() => requestLoadPresetByIndex(i)}
                style={{
                  fontWeight: i === currentPresetIndex ? 700 : 400,
                  color: i === currentPresetIndex ? colors.primary : colors.text,
                }}
              >
                {file.replace('.stellarr', '')}
              </MenuItem>
            ))
          )}
          <DropdownMenu.Separator
            style={{ height: 1, background: colors.border, margin: '0.25rem 0' }}
          />
          <MenuItem onSelect={requestNewSession} style={{ color: colors.muted }}>
            + New Preset
          </MenuItem>
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}

// -- Scene dropdown -----------------------------------------------------------

function SceneDropdown({
  currentName,
  scenes,
  activeSceneIndex,
}: {
  currentName: string;
  scenes: { name: string }[];
  activeSceneIndex: number;
}) {
  const [renameOpen, setRenameOpen] = useState(false);
  const [renamingIndex, setRenamingIndex] = useState(0);
  const [renameValue, setRenameValue] = useState('');

  const startRename = (i: number) => {
    setRenamingIndex(i);
    setRenameValue(scenes[i].name);
    setRenameOpen(true);
  };

  const submitRename = () => {
    if (renameValue.trim()) {
      requestRenameScene(renamingIndex, renameValue.trim());
    }
    setRenameOpen(false);
  };

  return (
    <>
      <SceneRenameDialog
        open={renameOpen}
        onOpenChange={setRenameOpen}
        value={renameValue}
        onChange={setRenameValue}
        onSubmit={submitRename}
      />
      <DropdownMenu.Root>
        <DropdownMenu.Trigger
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
          <DropdownTriggerContent
            label="Scene"
            value={currentName}
            hasValue={activeSceneIndex >= 0}
          />
        </DropdownMenu.Trigger>
        <DropdownMenu.Portal>
          <DropdownMenu.Content sideOffset={4} style={dropdownContentStyle}>
            {scenes.map((scene, i) => (
              <div key={i} style={{ display: 'flex', alignItems: 'center' }}>
                <MenuItem
                  onSelect={() => requestRecallScene(i)}
                  style={{
                    flex: 1,
                    fontWeight: i === activeSceneIndex ? 700 : 400,
                    color: i === activeSceneIndex ? colors.primary : colors.text,
                  }}
                >
                  {scene.name}
                </MenuItem>
                <DropdownMenu.Sub>
                  <DropdownMenu.SubTrigger
                    style={{
                      padding: '0.35rem 0.5rem',
                      color: colors.muted,
                      cursor: 'pointer',
                      outline: 'none',
                      display: 'flex',
                      alignItems: 'center',
                    }}
                    {...menuHover}
                  >
                    <DotsHorizontalIcon width={14} height={14} />
                  </DropdownMenu.SubTrigger>
                  <DropdownMenu.Portal>
                    <DropdownMenu.SubContent
                      sideOffset={4}
                      style={{ ...dropdownContentStyle, minWidth: 100, zIndex: 21 }}
                    >
                      <MenuItem
                        onSelect={(e) => {
                          e.preventDefault();
                          startRename(i);
                        }}
                      >
                        Rename
                      </MenuItem>
                      {scenes.length > 1 && (
                        <MenuItem
                          onSelect={() => requestDeleteScene(i)}
                          style={{ color: '#cc4444' }}
                        >
                          Delete
                        </MenuItem>
                      )}
                    </DropdownMenu.SubContent>
                  </DropdownMenu.Portal>
                </DropdownMenu.Sub>
              </div>
            ))}

            {scenes.length > 0 && (
              <DropdownMenu.Separator
                style={{ height: 1, background: colors.border, margin: '0.25rem 0' }}
              />
            )}

            {scenes.length < 16 && (
              <MenuItem onSelect={requestAddScene} style={{ color: colors.muted }}>
                + Add Scene
              </MenuItem>
            )}
          </DropdownMenu.Content>
        </DropdownMenu.Portal>
      </DropdownMenu.Root>
    </>
  );
}
