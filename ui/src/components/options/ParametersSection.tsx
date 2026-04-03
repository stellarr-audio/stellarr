import { useState } from 'react';
import { Select, Dialog } from 'radix-ui';
import { ChevronDownIcon } from '@radix-ui/react-icons';
import { Slider } from '../common/Slider';
import { useStore } from '../../store';
import {
  requestSetBlockMix,
  requestSetBlockBalance,
  requestSetBlockLevel,
  requestSetBlockBypassMode,
  requestStartMidiLearn,
  requestCancelMidiLearn,
  requestAddMidiMapping,
  requestRemoveMidiMapping,
} from '../../bridge';
import { colors } from '../common/colors';
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
  const learning = useStore((s) => s.midiLearning);
  const mappings = useStore((s) => s.midiMappings);
  const [dialogOpen, setDialogOpen] = useState(false);
  const [ccValue, setCcValue] = useState('');
  const [channel, setChannel] = useState('-1');

  // Find existing mapping for this target + block
  const existingIndex = mappings.findIndex(
    (m) => m.target === target && (m.blockId ?? '') === blockId,
  );
  const existing = existingIndex >= 0 ? mappings[existingIndex] : null;

  const openDialog = () => {
    if (existing) {
      setCcValue(String(existing.cc));
      setChannel(String(existing.channel));
    } else {
      setCcValue('');
      setChannel('-1');
    }
    setDialogOpen(true);
  };

  const submitManual = () => {
    const cc = parseInt(ccValue, 10);
    if (isNaN(cc) || cc < 0 || cc > 127) return;
    // Remove existing mapping first if updating
    if (existingIndex >= 0) requestRemoveMidiMapping(existingIndex);
    requestAddMidiMapping(parseInt(channel, 10), cc, target, blockId);
    setDialogOpen(false);
  };

  const clearMapping = () => {
    if (existingIndex >= 0) requestRemoveMidiMapping(existingIndex);
    setDialogOpen(false);
  };

  const midiLabel = existing
    ? `CC${existing.cc}${existing.channel >= 0 ? `/Ch${existing.channel + 1}` : ''}`
    : 'MIDI';

  return (
    <>
      <span style={{ display: 'flex', alignItems: 'center', gap: '0.3rem' }}>
        <span
          style={{
            fontSize: '1rem',
            color: colors.text,
            letterSpacing: '0.05em',
            textTransform: 'uppercase',
          }}
        >
          {label}
        </span>
        <button
          onClick={openDialog}
          title={existing ? `MIDI: CC ${existing.cc}` : `Assign MIDI CC to ${label}`}
          style={{
            background: existing ? `${colors.secondary}18` : 'transparent',
            border: `1px solid ${existing ? colors.secondary : colors.border}`,
            color: existing ? colors.secondary : colors.muted,
            fontSize: '0.65rem',
            fontWeight: 600,
            letterSpacing: '0.04em',
            textTransform: 'uppercase',
            cursor: 'pointer',
            padding: '0.1rem 0.3rem',
          }}
        >
          {midiLabel}
        </button>
      </span>

      <Dialog.Root open={dialogOpen} onOpenChange={setDialogOpen}>
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
              minWidth: 280,
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
              MIDI — {label}
            </Dialog.Title>

            <div style={{ display: 'flex', gap: '0.5rem', alignItems: 'center' }}>
              <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem', flex: 1 }}>
                <span
                  style={{ fontSize: '0.8rem', color: colors.muted, textTransform: 'uppercase' }}
                >
                  CC Number
                </span>
                <input
                  autoFocus
                  placeholder="0-127"
                  value={ccValue}
                  onChange={(e) => setCcValue(e.target.value.replace(/\D/g, '').slice(0, 3))}
                  onKeyDown={(e) => {
                    if (e.key === 'Enter') submitManual();
                  }}
                  style={{
                    background: colors.bg,
                    border: `1px solid ${colors.border}`,
                    color: colors.text,
                    fontSize: '1rem',
                    padding: '0.4rem 0.5rem',
                    outline: 'none',
                  }}
                />
              </div>
              <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem' }}>
                <span
                  style={{ fontSize: '0.8rem', color: colors.muted, textTransform: 'uppercase' }}
                >
                  Channel
                </span>
                <select
                  value={channel}
                  onChange={(e) => setChannel(e.target.value)}
                  style={{
                    background: colors.bg,
                    border: `1px solid ${colors.border}`,
                    color: colors.text,
                    fontSize: '1rem',
                    padding: '0.4rem 0.3rem',
                    outline: 'none',
                  }}
                >
                  <option value="-1">Any</option>
                  {Array.from({ length: 16 }, (_, i) => (
                    <option key={i} value={String(i)}>
                      {i + 1}
                    </option>
                  ))}
                </select>
              </div>
            </div>

            <div style={{ display: 'flex', gap: '0.5rem', justifyContent: 'space-between' }}>
              <button
                onClick={() => {
                  if (learning) requestCancelMidiLearn();
                  else requestStartMidiLearn(target, blockId);
                }}
                style={{
                  background: learning ? colors.primary : 'transparent',
                  border: `1px solid ${learning ? colors.primary : colors.border}`,
                  color: learning ? '#fff' : colors.muted,
                  padding: '0.35rem 0.75rem',
                  fontSize: '0.85rem',
                  fontWeight: 600,
                  cursor: 'pointer',
                }}
              >
                {learning ? 'Listening...' : 'Learn'}
              </button>

              <div style={{ display: 'flex', gap: '0.5rem' }}>
                {existing && (
                  <button
                    onClick={clearMapping}
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
                <button
                  onClick={() => setDialogOpen(false)}
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
                  onClick={submitManual}
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
    </>
  );
}

const bypassModes = [
  { value: 'thru', label: 'Thru' },
  { value: 'mute', label: 'Mute' },
];

interface Props {
  block: GridBlock;
}

export function ParametersSection({ block }: Props) {
  return (
    <>
      <div style={{ height: 1, background: colors.border }} />
      <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem' }}>
        <div
          style={{
            fontSize: '1rem',
            fontWeight: 600,
            color: colors.secondary,
            letterSpacing: '0.08em',
            textTransform: 'uppercase',
          }}
        >
          Parameters
        </div>

        {/* Mix */}
        <div>
          <div
            style={{
              display: 'flex',
              justifyContent: 'space-between',
              alignItems: 'center',
              marginBottom: '0.4rem',
            }}
          >
            <ParamLabel label="Mix" blockId={block.id} target="blockMix" />
            <span
              style={{
                fontSize: '1rem',
                color: colors.secondary,
                fontWeight: 600,
                fontVariantNumeric: 'tabular-nums',
                minWidth: '4ch',
                textAlign: 'right',
              }}
            >
              {Math.round((block.mix ?? 1) * 100)}%
            </span>
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
          <div
            style={{
              display: 'flex',
              justifyContent: 'space-between',
              alignItems: 'center',
              marginBottom: '0.4rem',
            }}
          >
            <ParamLabel label="Balance" blockId={block.id} target="blockBalance" />
            <span
              style={{
                fontSize: '1rem',
                color: colors.secondary,
                fontWeight: 600,
                fontVariantNumeric: 'tabular-nums',
                minWidth: '4ch',
                textAlign: 'right',
              }}
            >
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

        {/* Level */}
        <div>
          <div
            style={{
              display: 'flex',
              justifyContent: 'space-between',
              alignItems: 'center',
              marginBottom: '0.4rem',
            }}
          >
            <ParamLabel label="Level" blockId={block.id} target="blockLevel" />
            <span
              style={{
                fontSize: '1rem',
                color: colors.secondary,
                fontWeight: 600,
                fontVariantNumeric: 'tabular-nums',
                minWidth: '5ch',
                textAlign: 'right',
              }}
            >
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

        {/* Bypass Mode */}
        <div>
          <div
            style={{
              display: 'flex',
              justifyContent: 'space-between',
              alignItems: 'center',
              marginBottom: '0.4rem',
            }}
          >
            <span
              style={{
                fontSize: '1rem',
                color: colors.text,
                letterSpacing: '0.05em',
                textTransform: 'uppercase',
              }}
            >
              Bypass Mode
            </span>
          </div>
          <Select.Root
            value={block.bypassMode ?? 'thru'}
            onValueChange={(v) => {
              useStore.getState().setBlockBypassMode(block.id, v);
              requestSetBlockBypassMode(block.id, v);
            }}
          >
            <Select.Trigger
              style={{
                width: '100%',
                textAlign: 'left',
                background: colors.cell,
                color: colors.text,
                border: `1px solid ${colors.border}`,
                padding: '0.35rem 0.5rem',
                fontSize: '1rem',
                cursor: 'pointer',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
                outline: 'none',
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
                  width: 'var(--radix-select-trigger-width)',
                  zIndex: 20,
                }}
              >
                <Select.Viewport>
                  {bypassModes.map((m) => (
                    <Select.Item
                      key={m.value}
                      value={m.value}
                      style={{
                        padding: '0.35rem 0.5rem',
                        fontSize: '1rem',
                        color: colors.text,
                        cursor: 'pointer',
                        outline: 'none',
                      }}
                      onMouseEnter={(e) => {
                        e.currentTarget.style.background = `${colors.border}88`;
                      }}
                      onMouseLeave={(e) => {
                        e.currentTarget.style.background = 'transparent';
                      }}
                    >
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
