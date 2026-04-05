import { useState, useEffect } from 'react';
import { Dialog, Select } from 'radix-ui';
import { MixerHorizontalIcon, ChevronDownIcon } from '@radix-ui/react-icons';
import { useStore } from '../../store';
import {
  requestStartMidiLearn,
  requestCancelMidiLearn,
  requestAddMidiMapping,
  requestRemoveMidiMapping,
} from '../../bridge';
import { PROGRAM_CHANGE_CC } from './constants';
import styles from './MidiAssignDialog.module.css';

interface Props {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  title: string;
  target: string;
  blockId?: string;
  existingIndex?: number;
  programChange?: boolean;
}

export function MidiAssignDialog({
  open,
  onOpenChange,
  title,
  target,
  blockId,
  existingIndex,
  programChange,
}: Props) {
  const learning = useStore((s) => s.midiLearning);
  const mappings = useStore((s) => s.midiMappings);

  const existing =
    existingIndex !== undefined && existingIndex >= 0 ? mappings[existingIndex] : null;

  const [ccValue, setCcValue] = useState('');
  const [channel, setChannel] = useState('-1');

  useEffect(() => {
    if (open && existing) {
      setCcValue(String(existing.cc));
      setChannel(String(existing.channel));
    } else if (open) {
      setCcValue('');
      setChannel('-1');
    }
  }, [open, existing]);

  const submit = () => {
    if (programChange) {
      if (existingIndex !== undefined && existingIndex >= 0)
        requestRemoveMidiMapping(existingIndex);
      requestAddMidiMapping(parseInt(channel, 10), PROGRAM_CHANGE_CC, target, blockId);
      onOpenChange(false);
      return;
    }
    const cc = parseInt(ccValue, 10);
    if (isNaN(cc) || cc < 0 || cc > 127) return;
    if (existingIndex !== undefined && existingIndex >= 0) requestRemoveMidiMapping(existingIndex);
    requestAddMidiMapping(parseInt(channel, 10), cc, target, blockId);
    onOpenChange(false);
  };

  const clear = () => {
    if (existingIndex !== undefined && existingIndex >= 0) requestRemoveMidiMapping(existingIndex);
    onOpenChange(false);
  };

  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Overlay className={styles.overlay} />
        <Dialog.Content className={styles.content}>
          <Dialog.Title className={styles.title}>{title}</Dialog.Title>

          <div className={styles.fieldRow}>
            {/* CC Number with Learn icon button — hidden in PC mode */}
            {!programChange && (
              <div className={styles.fieldGroupFlex}>
                <span className={styles.fieldLabel}>CC Number</span>
                <div className={`${styles.ccInputWrap} ${learning ? styles.learning : ''}`}>
                  <button
                    onClick={() => {
                      if (learning) requestCancelMidiLearn();
                      else requestStartMidiLearn(target, blockId);
                    }}
                    title={
                      learning
                        ? 'Cancel MIDI learn'
                        : 'Learn — send a CC from your controller to auto-detect'
                    }
                    className={`${styles.learnButton} ${learning ? styles.learning : ''}`}
                  >
                    <MixerHorizontalIcon width={16} height={16} />
                  </button>
                  <input
                    autoFocus
                    placeholder="0-127"
                    value={ccValue}
                    onChange={(e) => setCcValue(e.target.value.replace(/\D/g, '').slice(0, 3))}
                    onKeyDown={(e) => {
                      if (e.key === 'Enter') submit();
                    }}
                    className={styles.ccInput}
                  />
                </div>
              </div>
            )}

            {/* Channel */}
            <div className={styles.fieldGroup}>
              <span className={styles.fieldLabel}>Channel</span>
              <Select.Root value={channel} onValueChange={setChannel}>
                <Select.Trigger className={styles.selectTrigger}>
                  <Select.Value />
                  <Select.Icon>
                    <ChevronDownIcon />
                  </Select.Icon>
                </Select.Trigger>
                <Select.Portal>
                  <Select.Content position="popper" sideOffset={4} className={styles.selectContent}>
                    <Select.Viewport>
                      <Select.Item value="-1" className={styles.selectItem}>
                        <Select.ItemText>Any</Select.ItemText>
                      </Select.Item>
                      {Array.from({ length: 16 }, (_, i) => (
                        <Select.Item key={i} value={String(i)} className={styles.selectItem}>
                          <Select.ItemText>Ch {i + 1}</Select.ItemText>
                        </Select.Item>
                      ))}
                    </Select.Viewport>
                  </Select.Content>
                </Select.Portal>
              </Select.Root>
            </div>
          </div>

          {programChange && (
            <p className={styles.pcNote}>
              Program Change value maps directly to preset index in the active folder.
            </p>
          )}

          {/* Buttons: Clear (left) | Cancel + Save (right) */}
          <div className={styles.buttonRow}>
            <div>
              {existing && (
                <button onClick={clear} className={styles.clearButton}>
                  Clear
                </button>
              )}
            </div>
            <div className={styles.buttonGroup}>
              <button onClick={() => onOpenChange(false)} className={styles.cancelButton}>
                Cancel
              </button>
              <button onClick={submit} className={styles.saveButton}>
                Save
              </button>
            </div>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
