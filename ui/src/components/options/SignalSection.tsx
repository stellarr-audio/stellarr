import { useState } from 'react';
import type { GridBlock } from '../../store';
import { useStore } from '../../store';
import { Slider } from '../common/Slider';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import { formatMidiLabel } from '../common/constants';
import { requestSetBlockLevel } from '../../bridge';
import { LoudnessHistory } from './LoudnessHistory';
import styles from './SignalSection.module.css';

interface Props {
  block: GridBlock;
}

function formatDb(db: number): string {
  if (db <= -60) return '-∞ dB';
  return `${db >= 0 ? '+' : ''}${db.toFixed(1)} dB`;
}

export function SignalSection({ block }: Props) {
  const level = block.level ?? 0;
  const mappings = useStore((s) => s.midiMappings);
  const [dialogOpen, setDialogOpen] = useState(false);

  const existingIndex = mappings.findIndex(
    (m) => m.target === 'blockLevel' && (m.blockId ?? '') === block.id,
  );
  const existing = existingIndex >= 0 ? mappings[existingIndex] : null;

  return (
    <div className={styles.section}>
      <div className={styles.header}>
        <span className={styles.labelRow}>
          <span className={styles.label}>Level</span>
          <button
            onClick={() => setDialogOpen(true)}
            title={existing ? `MIDI: CC ${existing.cc}` : 'Assign MIDI CC to Level'}
            className={`${styles.midiButton} ${existing ? styles.midiButtonAssigned : ''}`}
          >
            {formatMidiLabel(existing)}
          </button>
        </span>
        <span className={styles.value}>{formatDb(level)}</span>
      </div>
      <Slider
        min={-60}
        max={12}
        step={0.1}
        value={level}
        onChange={(v) => {
          useStore.getState().setBlockLevel(block.id, v);
          requestSetBlockLevel(block.id, v);
        }}
      />
      <LoudnessHistory blockId={block.id} />
      <MidiAssignDialog
        open={dialogOpen}
        onOpenChange={setDialogOpen}
        title="MIDI — Level"
        target="blockLevel"
        blockId={block.id}
        existingIndex={existingIndex >= 0 ? existingIndex : undefined}
      />
    </div>
  );
}
