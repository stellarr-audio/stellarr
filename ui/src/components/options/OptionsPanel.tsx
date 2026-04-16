import { useState, useEffect, useMemo } from 'react';
import { useStore } from '../../store';
import { ToggleSwitch } from '../common/ToggleSwitch';
import { Tooltip } from '../common/Tooltip';
import { Input } from '../common/Input';
import { Button } from '../common/Button';
import { IconButton } from '../common/IconButton';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import { TYPE_ABBREVIATIONS, formatMidiLabel } from '../common/constants';
import { Pencil1Icon, PlayIcon, StopIcon, FrameIcon } from '@radix-ui/react-icons';
import { ColorPicker } from './ColorPicker';
import { PluginSection } from './PluginSection';
import { ParametersSection } from './ParametersSection';
import { StatesSection } from './StatesSection';
import { SignalSection } from './SignalSection';
import { TargetLoudnessControl } from './TargetLoudnessControl';
import { Select } from 'radix-ui';
import { ChevronDownIcon } from '@radix-ui/react-icons';
import {
  requestToggleTestTone,
  requestToggleBlockBypass,
  requestRenameBlock,
  requestSetBlockColor,
  requestGetTestToneSamples,
  requestSetTestToneSample,
} from '../../bridge';
import styles from './OptionsPanel.module.css';

export function OptionsPanel() {
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const blocks = useStore((s) => s.blocks);
  const availablePlugins = useStore((s) => s.availablePlugins);

  const block = selectedBlockId ? blocks.find((b) => b.id === selectedBlockId) : null;

  return (
    <div className={styles.panel}>
      <div className={styles.title}>Options</div>

      {!block ? (
        <div className={styles.emptyMessage}>Select a block to view options</div>
      ) : (
        <div className={styles.content}>
          <BlockHeader block={block} />

          <div className={styles.divider} />

          {/* Signal — Level slider + loudness history. Same for every block. */}
          <SignalSection block={block} />

          {/* Input block — test tone picker */}
          {block.type === 'input' && (
            <>
              <div className={styles.divider} />
              <TestToneSamplePicker blockId={block.id} playing={block.testTone ?? false} />
            </>
          )}

          {/* Plugin block — plugin select */}
          {block.type === 'plugin' && (
            <>
              <div className={styles.divider} />
              <PluginSection block={block} availablePlugins={availablePlugins} />
            </>
          )}

          {/* Plugin block — parameters (Mix, Balance, Bypass mode) */}
          {block.type === 'plugin' && <ParametersSection block={block} />}

          {/* Output block — target loudness */}
          {block.type === 'output' && (
            <>
              <div className={styles.divider} />
              <TargetLoudnessControl blockId={block.id} />
            </>
          )}

          {/* Plugin block — states */}
          {block.type === 'plugin' && <StatesSection block={block} />}
        </div>
      )}
    </div>
  );
}

function BlockHeader({ block }: { block: import('../../store').GridBlock }) {
  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState('');
  const [idCopied, setIdCopied] = useState(false);

  const abbreviation = TYPE_ABBREVIATIONS[block.type] || block.type.slice(0, 3).toUpperCase();
  const displayName = block.displayName || abbreviation;

  const startEdit = () => {
    setEditValue(displayName);
    setEditing(true);
  };

  const submitEdit = () => {
    setEditing(false);
    const trimmed = editValue.trim();
    if (trimmed && trimmed !== displayName) {
      requestRenameBlock(block.id, trimmed);
    }
  };

  const copyId = () => {
    navigator.clipboard.writeText(block.id);
    setIdCopied(true);
    setTimeout(() => setIdCopied(false), 1200);
  };

  return (
    <div className={styles.blockHeader}>
      {editing ? (
        <Input
          autoFocus
          maxLength={3}
          value={editValue}
          onChange={(e) => setEditValue(e.target.value)}
          onBlur={submitEdit}
          onKeyDown={(e) => {
            if (e.key === 'Enter') submitEdit();
            if (e.key === 'Escape') setEditing(false);
          }}
          className={styles.editInput}
        />
      ) : (
        <div className={styles.nameRow}>
          <ColorPicker
            color={block.blockColor}
            onChange={(color) => requestSetBlockColor(block.id, color)}
          />
          <span className={`${styles.blockName} ${block.bypassed ? styles.blockNameBypassed : ''}`}>
            {displayName}
          </span>
          <Tooltip content="Rename block">
            <span className={styles.iconButton} onClick={startEdit}>
              <Pencil1Icon width={14} height={14} />
            </span>
          </Tooltip>
          <Tooltip
            open={idCopied ? true : undefined}
            content={idCopied ? 'Block ID copied' : 'Copy block ID'}
          >
            <span
              className={`${styles.iconButton} ${idCopied ? styles.iconButtonCopied : ''}`}
              onClick={copyId}
            >
              <FrameIcon width={14} height={14} />
            </span>
          </Tooltip>
        </div>
      )}

      {/* Bypass toggle + MIDI — non-I/O blocks only */}
      {block.type !== 'input' && block.type !== 'output' && <BypassControl block={block} />}
    </div>
  );
}

function BypassControl({ block }: { block: import('../../store').GridBlock }) {
  const mappings = useStore((s) => s.midiMappings);
  const [dialogOpen, setDialogOpen] = useState(false);

  const existingIndex = mappings.findIndex(
    (m) => m.target === 'blockBypass' && (m.blockId ?? '') === block.id,
  );
  const existing = existingIndex >= 0 ? mappings[existingIndex] : null;

  const midiLabel = formatMidiLabel(existing);

  return (
    <div className={styles.bypassControl}>
      <Button
        size="sm"
        variant="secondary"
        active={!!existing}
        onClick={() => setDialogOpen(true)}
        title={existing ? `Bypass MIDI: CC ${existing.cc}` : 'Assign MIDI CC to bypass'}
      >
        {midiLabel}
      </Button>
      <ToggleSwitch
        enabled={!block.bypassed}
        onToggle={() => {
          useStore.getState().setBlockBypassed(block.id, !block.bypassed);
          requestToggleBlockBypass(block.id);
        }}
        title={block.bypassed ? 'Enable block' : 'Bypass block'}
      />
      <MidiAssignDialog
        open={dialogOpen}
        onOpenChange={setDialogOpen}
        title="MIDI — Bypass"
        target="blockBypass"
        blockId={block.id}
        existingIndex={existingIndex >= 0 ? existingIndex : undefined}
      />
    </div>
  );
}

function TestToneSamplePicker({ blockId, playing }: { blockId: string; playing: boolean }) {
  const samples = useStore((s) => s.testToneSamples);
  const sortedSamples = useMemo(() => samples.toSorted((a, b) => a.localeCompare(b)), [samples]);
  const block = useStore((s) => s.blocks.find((b) => b.id === blockId));
  const currentSample = block?.testToneSample || 'Synth (Default)';

  // Fetch samples list on mount
  useEffect(() => {
    requestGetTestToneSamples();
  }, []);

  if (samples.length === 0) return null;

  return (
    <div className={styles.samplePicker}>
      <span className={styles.sampleLabel}>Test Tone</span>
      <div className={styles.sampleRow}>
        <Select.Root
          value={currentSample}
          onValueChange={(v) => requestSetTestToneSample(blockId, v)}
        >
          <Select.Trigger className={styles.sampleTrigger}>
            <Select.Value />
            <Select.Icon>
              <ChevronDownIcon />
            </Select.Icon>
          </Select.Trigger>
          <Select.Portal>
            <Select.Content position="popper" sideOffset={4} className={styles.sampleContent}>
              <Select.Viewport>
                {sortedSamples.map((s) => (
                  <Select.Item key={s} value={s} className={styles.sampleItem}>
                    <Select.ItemText>{s}</Select.ItemText>
                  </Select.Item>
                ))}
              </Select.Viewport>
            </Select.Content>
          </Select.Portal>
        </Select.Root>
        <IconButton
          icon={playing ? <StopIcon width={14} height={14} /> : <PlayIcon width={14} height={14} />}
          onClick={() => requestToggleTestTone(blockId)}
          title={playing ? 'Stop test tone' : 'Play test tone'}
          className={playing ? styles.toneButtonPlaying : undefined}
        />
      </div>
    </div>
  );
}
