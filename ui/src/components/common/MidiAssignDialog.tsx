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
import { colors } from './colors';

interface Props {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  title: string;
  target: string;
  blockId?: string;
  existingIndex?: number;
}

export function MidiAssignDialog({
  open,
  onOpenChange,
  title,
  target,
  blockId,
  existingIndex,
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
        <Dialog.Overlay
          style={{
            position: 'fixed',
            inset: 0,
            background: 'rgba(0,0,0,0.5)',
            zIndex: 50,
          }}
        />
        <Dialog.Content
          style={{
            position: 'fixed',
            top: '50%',
            left: '50%',
            transform: 'translate(-50%, -50%)',
            background: colors.dropdownBg,
            border: `1px solid ${colors.border}`,
            padding: '1.5rem',
            zIndex: 51,
            minWidth: 300,
            display: 'flex',
            flexDirection: 'column',
            gap: '1rem',
          }}
        >
          <Dialog.Title
            style={{
              fontSize: '1rem',
              fontWeight: 700,
              color: colors.text,
              letterSpacing: '0.06em',
              textTransform: 'uppercase',
              margin: 0,
            }}
          >
            {title}
          </Dialog.Title>

          <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'flex-end' }}>
            {/* CC Number with Learn icon button */}
            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem', flex: 1 }}>
              <span style={{ fontSize: '0.8rem', color: colors.muted, textTransform: 'uppercase' }}>
                CC Number
              </span>
              <div
                style={{
                  display: 'flex',
                  alignItems: 'stretch',
                  border: `1px solid ${learning ? colors.primary : colors.border}`,
                }}
              >
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
                  style={{
                    background: learning ? colors.primary : 'transparent',
                    border: 'none',
                    borderRight: `1px solid ${learning ? colors.primary : colors.border}`,
                    color: learning ? '#ffffff' : colors.muted,
                    padding: '0.4rem',
                    cursor: 'pointer',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                    transition: 'background 0.15s ease',
                  }}
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
                  style={{
                    flex: 1,
                    background: colors.bg,
                    border: 'none',
                    color: colors.text,
                    fontSize: '1rem',
                    padding: '0.4rem 0.5rem',
                    outline: 'none',
                  }}
                />
              </div>
            </div>

            {/* Channel */}
            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem' }}>
              <span style={{ fontSize: '0.8rem', color: colors.muted, textTransform: 'uppercase' }}>
                Channel
              </span>
              <Select.Root value={channel} onValueChange={setChannel}>
                <Select.Trigger
                  style={{
                    background: colors.bg,
                    border: `1px solid ${colors.border}`,
                    color: colors.text,
                    fontSize: '1rem',
                    padding: '0.4rem 0.5rem',
                    cursor: 'pointer',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    gap: '0.3rem',
                    outline: 'none',
                    minWidth: 70,
                  }}
                >
                  <Select.Value />
                  <Select.Icon>
                    <ChevronDownIcon />
                  </Select.Icon>
                </Select.Trigger>
                <Select.Portal>
                  <Select.Content
                    position="popper"
                    sideOffset={4}
                    style={{
                      background: colors.dropdownBg,
                      border: `1px solid ${colors.border}`,
                      maxHeight: 200,
                      overflowY: 'auto',
                      zIndex: 52,
                    }}
                  >
                    <Select.Viewport>
                      <Select.Item
                        value="-1"
                        style={{
                          padding: '0.35rem 0.5rem',
                          fontSize: '1rem',
                          color: colors.text,
                          cursor: 'pointer',
                          outline: 'none',
                        }}
                      >
                        <Select.ItemText>Any</Select.ItemText>
                      </Select.Item>
                      {Array.from({ length: 16 }, (_, i) => (
                        <Select.Item
                          key={i}
                          value={String(i)}
                          style={{
                            padding: '0.35rem 0.5rem',
                            fontSize: '1rem',
                            color: colors.text,
                            cursor: 'pointer',
                            outline: 'none',
                          }}
                        >
                          <Select.ItemText>Ch {i + 1}</Select.ItemText>
                        </Select.Item>
                      ))}
                    </Select.Viewport>
                  </Select.Content>
                </Select.Portal>
              </Select.Root>
            </div>
          </div>

          {/* Buttons: Clear (left) | Cancel + Save (right) */}
          <div style={{ display: 'flex', justifyContent: 'space-between' }}>
            <div>
              {existing && (
                <button
                  onClick={clear}
                  style={{
                    background: 'transparent',
                    border: `1px solid ${colors.danger}`,
                    color: colors.danger,
                    padding: '0.35rem 0.75rem',
                    fontSize: '0.85rem',
                    cursor: 'pointer',
                  }}
                >
                  Clear
                </button>
              )}
            </div>
            <div style={{ display: 'flex', gap: '0.5rem' }}>
              <button
                onClick={() => onOpenChange(false)}
                style={{
                  background: 'transparent',
                  border: `1px solid ${colors.border}`,
                  color: colors.muted,
                  padding: '0.35rem 0.75rem',
                  fontSize: '0.85rem',
                  cursor: 'pointer',
                }}
              >
                Cancel
              </button>
              <button
                onClick={submit}
                style={{
                  background: colors.primary,
                  border: 'none',
                  color: '#ffffff',
                  padding: '0.35rem 0.75rem',
                  fontSize: '0.85rem',
                  fontWeight: 600,
                  cursor: 'pointer',
                }}
              >
                Save
              </button>
            </div>
          </div>
        </Dialog.Content>
      </Dialog.Portal>
    </Dialog.Root>
  );
}
