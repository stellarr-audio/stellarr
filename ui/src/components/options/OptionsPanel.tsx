import { useState, useEffect } from 'react';
import { useStore } from '../../store';
import { Slider } from '../common/Slider';
import { ToggleSwitch } from '../common/ToggleSwitch';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import { TYPE_ABBREVIATIONS, formatMidiLabel } from '../common/constants';
import { Pencil1Icon, PlayIcon, StopIcon } from '@radix-ui/react-icons';
import { ColorPicker } from './ColorPicker';
import { MetricsSection } from './MetricsSection';
import { PluginSection } from './PluginSection';
import { ParametersSection } from './ParametersSection';
import { StatesSection } from './StatesSection';
import { LoudnessHistory } from './LoudnessHistory';
import { TargetLoudnessControl } from './TargetLoudnessControl';
import { Select } from 'radix-ui';
import { ChevronDownIcon } from '@radix-ui/react-icons';
import {
  requestToggleTestTone,
  requestToggleBlockBypass,
  requestSetBlockLevel,
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

          <MetricsSection blockId={block.id} />

          <div className={styles.divider} />

          {/* Input block options */}
          {block.type === 'input' && (
            <TestToneSamplePicker blockId={block.id} playing={block.testTone ?? false} />
          )}

          {/* Plugin block — plugin select */}
          {block.type === 'plugin' && (
            <PluginSection block={block} availablePlugins={availablePlugins} />
          )}

          {/* Level — for I/O blocks (plugin blocks get it via ParametersSection) */}
          {(block.type === 'input' || block.type === 'output') && (
            <>
              <div className={styles.divider} />
              <div>
                <div className={styles.levelRow}>
                  <span className={styles.levelLabel}>Level</span>
                  <span className={styles.levelValue}>
                    {(() => {
                      const db = block.level ?? 0;
                      if (db <= -60) return '-∞ dB';
                      return `${db >= 0 ? '+' : ''}${db.toFixed(1)} dB`;
                    })()}
                  </span>
                </div>
                <Slider
                  min={-60}
                  max={12}
                  step={0.1}
                  value={block.level ?? 0}
                  onChange={(v) => {
                    useStore.getState().setBlockLevel(block.id, v);
                    requestSetBlockLevel(block.id, v);
                  }}
                />
              </div>
            </>
          )}

          {/* Target Loudness — Output block only */}
          {block?.type === 'output' && (
            <>
              <div className={styles.divider} />
              <TargetLoudnessControl blockId={block.id} />
            </>
          )}

          {/* Parameters — for non-I/O blocks */}
          {block.type !== 'input' && block.type !== 'output' && <ParametersSection block={block} />}

          {/* States — plugin blocks only */}
          {block.type === 'plugin' && <StatesSection block={block} />}

          {block && <LoudnessHistory blockId={block.id} />}

          <div
            className={styles.blockId}
            title="Click to copy block ID"
            onClick={() => navigator.clipboard.writeText(block.id)}
          >
            {block.id}
          </div>
        </div>
      )}
    </div>
  );
}

function BlockHeader({ block }: { block: import('../../store').GridBlock }) {
  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState('');

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

  return (
    <div className={styles.blockHeader}>
      {editing ? (
        <input
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
          <Pencil1Icon width={14} height={14} className={styles.editIcon} onClick={startEdit} />
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
      <button
        onClick={() => setDialogOpen(true)}
        title={existing ? `Bypass MIDI: CC ${existing.cc}` : 'Assign MIDI CC to bypass'}
        className={`${styles.midiButton} ${existing ? styles.midiButtonAssigned : ''}`}
      >
        {midiLabel}
      </button>
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
                {samples.map((s) => (
                  <Select.Item key={s} value={s} className={styles.sampleItem}>
                    <Select.ItemText>{s}</Select.ItemText>
                  </Select.Item>
                ))}
              </Select.Viewport>
            </Select.Content>
          </Select.Portal>
        </Select.Root>
        <button
          onClick={() => requestToggleTestTone(blockId)}
          title={playing ? 'Stop test tone' : 'Play test tone'}
          className={`${styles.toneButton} ${playing ? styles.toneButtonPlaying : ''}`}
        >
          {playing ? <StopIcon width={14} height={14} /> : <PlayIcon width={14} height={14} />}
        </button>
      </div>
    </div>
  );
}
