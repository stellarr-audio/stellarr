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
  requestRenamePreset,
  requestDeletePreset,
  requestAddScene,
  requestRecallScene,
  requestRenameScene,
  requestDeleteScene,
} from '../../bridge';
import { SceneRenameDialog } from './SceneRenameDialog';
import { ConfirmDialog } from './ConfirmDialog';
import styles from './PresetBrowser.module.css';

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
      <span className={styles.triggerLabel}>{label}</span>
      <span className={styles.triggerValue}>
        <span
          className={`${styles.triggerValueText} ${hasValue ? styles.hasValue : styles.noValue}`}
        >
          {value}
        </span>
        <ChevronDownIcon width={12} height={12} className={styles.triggerChevron} />
      </span>
    </>
  );
}

// -- Menu item wrapper --------------------------------------------------------

function MenuItem({
  onSelect,
  className,
  children,
}: {
  onSelect: (e: Event) => void;
  className?: string;
  children: React.ReactNode;
}) {
  return (
    <DropdownMenu.Item onSelect={onSelect} className={className ?? styles.menuItem}>
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
    <div className={styles.container}>
      {/* Open */}
      <button onClick={requestLoadSession} title="Open preset" className={styles.iconBtn}>
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
      <div className={styles.saveSplit}>
        <button
          onClick={requestSaveSessionQuiet}
          title="Save preset"
          className={justSaved ? styles.saveBtnSaved : styles.saveBtnDefault}
        >
          {justSaved ? (
            <CheckIcon width={16} height={16} />
          ) : (
            <BookmarkIcon width={16} height={16} />
          )}
        </button>

        <DropdownMenu.Root>
          <DropdownMenu.Trigger asChild>
            <button title="Save options" className={styles.saveChevron}>
              <ChevronDownIcon width={12} height={12} />
            </button>
          </DropdownMenu.Trigger>
          <DropdownMenu.Portal>
            <DropdownMenu.Content
              sideOffset={4}
              align="end"
              className={styles.dropdownContentNarrow}
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
  const [renameOpen, setRenameOpen] = useState(false);
  const [renamingIndex, setRenamingIndex] = useState(0);
  const [renameValue, setRenameValue] = useState('');
  const [deleteOpen, setDeleteOpen] = useState(false);
  const [deletingIndex, setDeletingIndex] = useState(0);

  const startRename = (i: number) => {
    setRenamingIndex(i);
    setRenameValue(presetFiles[i].replace('.stellarr', ''));
    setRenameOpen(true);
  };

  const submitRename = () => {
    if (renameValue.trim()) {
      requestRenamePreset(renamingIndex, renameValue.trim());
    }
    setRenameOpen(false);
  };

  const startDelete = (i: number) => {
    setDeletingIndex(i);
    setDeleteOpen(true);
  };

  const confirmDelete = () => {
    requestDeletePreset(deletingIndex);
    setDeleteOpen(false);
  };

  const deleteName =
    deletingIndex >= 0 && deletingIndex < presetFiles.length
      ? presetFiles[deletingIndex].replace('.stellarr', '')
      : '';

  return (
    <>
      <SceneRenameDialog
        open={renameOpen}
        onOpenChange={setRenameOpen}
        title="Rename Preset"
        value={renameValue}
        onChange={setRenameValue}
        onSubmit={submitRename}
      />
      <ConfirmDialog
        open={deleteOpen}
        onOpenChange={setDeleteOpen}
        title="Delete Preset"
        message={`Are you sure you want to delete "${deleteName}"? This cannot be undone.`}
        onConfirm={confirmDelete}
      />
      <DropdownMenu.Root>
        <DropdownMenu.Trigger className={styles.dropdownTrigger}>
          <DropdownTriggerContent
            label="Preset"
            value={currentName}
            hasValue={currentPresetIndex >= 0}
          />
        </DropdownMenu.Trigger>
        <DropdownMenu.Portal>
          <DropdownMenu.Content sideOffset={4} className={styles.dropdownContentScrollable}>
            {presetFiles.length === 0 ? (
              <div className={styles.emptyState}>No presets</div>
            ) : (
              presetFiles.map((file, i) => (
                <div key={i} className={styles.sceneRow}>
                  <MenuItem
                    onSelect={() => requestLoadPresetByIndex(i)}
                    className={
                      i === currentPresetIndex ? styles.menuItemActiveFlex : styles.menuItemFlex
                    }
                  >
                    {file.replace('.stellarr', '')}
                  </MenuItem>
                  <DropdownMenu.Sub>
                    <DropdownMenu.SubTrigger className={styles.subTrigger}>
                      <DotsHorizontalIcon width={14} height={14} />
                    </DropdownMenu.SubTrigger>
                    <DropdownMenu.Portal>
                      <DropdownMenu.SubContent sideOffset={4} className={styles.subContent}>
                        <MenuItem
                          onSelect={(e) => {
                            e.preventDefault();
                            startRename(i);
                          }}
                        >
                          Rename
                        </MenuItem>
                        <MenuItem
                          onSelect={(e) => {
                            e.preventDefault();
                            startDelete(i);
                          }}
                          className={styles.menuItemDanger}
                        >
                          Delete
                        </MenuItem>
                      </DropdownMenu.SubContent>
                    </DropdownMenu.Portal>
                  </DropdownMenu.Sub>
                </div>
              ))
            )}
            <DropdownMenu.Separator className={styles.separator} />
            <MenuItem onSelect={requestNewSession} className={styles.menuItemMuted}>
              + New Preset
            </MenuItem>
          </DropdownMenu.Content>
        </DropdownMenu.Portal>
      </DropdownMenu.Root>
    </>
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
        <DropdownMenu.Trigger className={styles.dropdownTrigger}>
          <DropdownTriggerContent
            label="Scene"
            value={currentName}
            hasValue={activeSceneIndex >= 0}
          />
        </DropdownMenu.Trigger>
        <DropdownMenu.Portal>
          <DropdownMenu.Content sideOffset={4} className={styles.dropdownContent}>
            {scenes.map((scene, i) => (
              <div key={i} className={styles.sceneRow}>
                <MenuItem
                  onSelect={() => requestRecallScene(i)}
                  className={
                    i === activeSceneIndex ? styles.menuItemActiveFlex : styles.menuItemFlex
                  }
                >
                  {scene.name}
                </MenuItem>
                <DropdownMenu.Sub>
                  <DropdownMenu.SubTrigger className={styles.subTrigger}>
                    <DotsHorizontalIcon width={14} height={14} />
                  </DropdownMenu.SubTrigger>
                  <DropdownMenu.Portal>
                    <DropdownMenu.SubContent sideOffset={4} className={styles.subContent}>
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
                          className={styles.menuItemDanger}
                        >
                          Delete
                        </MenuItem>
                      )}
                    </DropdownMenu.SubContent>
                  </DropdownMenu.Portal>
                </DropdownMenu.Sub>
              </div>
            ))}

            {scenes.length > 0 && <DropdownMenu.Separator className={styles.separator} />}

            {scenes.length < 16 && (
              <MenuItem onSelect={requestAddScene} className={styles.menuItemMuted}>
                + Add Scene
              </MenuItem>
            )}
          </DropdownMenu.Content>
        </DropdownMenu.Portal>
      </DropdownMenu.Root>
    </>
  );
}
