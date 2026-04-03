import { Select } from 'radix-ui';
import { ChevronDownIcon } from '@radix-ui/react-icons';
import { Slider } from '../common/Slider';
import { useStore } from '../../store';
import {
  requestSetBlockMix,
  requestSetBlockBalance,
  requestSetBlockLevel,
  requestSetBlockBypassMode,
} from '../../bridge';
import { colors } from '../common/colors';
import type { GridBlock } from '../../store';

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
            <span
              style={{
                fontSize: '1rem',
                color: colors.text,
                letterSpacing: '0.05em',
                textTransform: 'uppercase',
              }}
            >
              Mix
            </span>
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
            <span
              style={{
                fontSize: '1rem',
                color: colors.text,
                letterSpacing: '0.05em',
                textTransform: 'uppercase',
              }}
            >
              Balance
            </span>
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
            <span
              style={{
                fontSize: '1rem',
                color: colors.text,
                letterSpacing: '0.05em',
                textTransform: 'uppercase',
              }}
            >
              Level
            </span>
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
