import { useState } from 'react';
import { Select } from 'radix-ui';
import { ChevronDownIcon } from '@radix-ui/react-icons';
import { Slider } from '../common/Slider';
import { Button } from '../common/Button';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import { formatMidiLabel } from '../common/constants';
import { useStore } from '../../store';
import {
  requestSetBlockMix,
  requestSetBlockBalance,
  requestSetBlockBypassMode,
} from '../../bridge';
import styles from './ParametersSection.module.css';
import type { GridBlock } from '../../store';

function ParamLabel({
  label,
  blockId,
  target,
}: {
  label: string;
  blockId: string;
  target: string;
}) {
  const mappings = useStore((s) => s.midiMappings);
  const [dialogOpen, setDialogOpen] = useState(false);

  const existingIndex = mappings.findIndex(
    (m) => m.target === target && (m.blockId ?? '') === blockId,
  );
  const existing = existingIndex >= 0 ? mappings[existingIndex] : null;

  const midiLabel = formatMidiLabel(existing);

  return (
    <>
      <span className={styles.paramLabelContainer}>
        <span className={styles.paramLabelText}>{label}</span>
        <Button
          size="sm"
          variant="secondary"
          active={!!existing}
          onClick={() => setDialogOpen(true)}
          title={existing ? `MIDI: CC ${existing.cc}` : `Assign MIDI CC to ${label}`}
        >
          {midiLabel}
        </Button>
      </span>

      <MidiAssignDialog
        open={dialogOpen}
        onOpenChange={setDialogOpen}
        title={`MIDI — ${label}`}
        target={target}
        blockId={blockId}
        existingIndex={existingIndex >= 0 ? existingIndex : undefined}
      />
    </>
  );
}

const bypassModes = [
  { value: 'thru', label: 'Thru' },
  { value: 'mute', label: 'Mute' },
  { value: 'muteIn', label: 'Mute In' },
  { value: 'muteOut', label: 'Mute Out' },
  { value: 'muteFxIn', label: 'Mute FX In' },
  { value: 'muteFxOut', label: 'Mute FX Out' },
];

interface Props {
  block: GridBlock;
}

export function ParametersSection({ block }: Props) {
  return (
    <>
      <div className={styles.divider} />
      <div className={styles.container}>
        <div className={styles.sectionTitle}>Parameters</div>

        {/* Mix */}
        <div>
          <div className={styles.paramRow}>
            <ParamLabel label="Mix" blockId={block.id} target="blockMix" />
            <span className={styles.paramValue}>{Math.round((block.mix ?? 1) * 100)}%</span>
          </div>
          <Slider
            value={Math.round((block.mix ?? 1) * 100)}
            defaultValue={100}
            onChange={(v) => {
              useStore.getState().setBlockMix(block.id, v / 100);
              requestSetBlockMix(block.id, v / 100);
            }}
          />
        </div>

        {/* Balance */}
        <div>
          <div className={styles.paramRow}>
            <ParamLabel label="Balance" blockId={block.id} target="blockBalance" />
            <span className={styles.paramValue}>
              {(() => {
                const bal = Math.round((block.balance ?? 0) * 100);
                if (bal === 0) return 'C';
                return bal < 0 ? `L${Math.abs(bal)}` : `R${bal}`;
              })()}
            </span>
          </div>
          <Slider
            min={-100}
            max={100}
            value={Math.round((block.balance ?? 0) * 100)}
            onChange={(v) => {
              useStore.getState().setBlockBalance(block.id, v / 100);
              requestSetBlockBalance(block.id, v / 100);
            }}
          />
        </div>

        {/* Bypass Mode */}
        <div>
          <div className={styles.paramRow}>
            <span className={styles.bypassModeLabel}>Bypass Mode</span>
          </div>
          <Select.Root
            value={block.bypassMode ?? 'thru'}
            onValueChange={(v) => {
              useStore.getState().setBlockBypassMode(block.id, v);
              requestSetBlockBypassMode(block.id, v);
            }}
          >
            <Select.Trigger className={styles.selectTrigger}>
              <Select.Value />
              <Select.Icon>
                <ChevronDownIcon />
              </Select.Icon>
            </Select.Trigger>
            <Select.Portal>
              <Select.Content position="popper" sideOffset={4} className={styles.selectContent}>
                <Select.Viewport>
                  {bypassModes.map((m) => (
                    <Select.Item key={m.value} value={m.value} className={styles.selectItem}>
                      <Select.ItemText>{m.label}</Select.ItemText>
                    </Select.Item>
                  ))}
                </Select.Viewport>
              </Select.Content>
            </Select.Portal>
          </Select.Root>
        </div>
      </div>
    </>
  );
}
