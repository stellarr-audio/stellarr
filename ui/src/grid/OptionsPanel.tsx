import { useStore } from '../store';
import { PluginSelect } from './PluginSelect';
import { Slider } from './Slider';
import { Select } from 'radix-ui';
import { GearIcon, ChevronDownIcon, PlusIcon, Cross2Icon } from '@radix-ui/react-icons';
import {
  requestToggleTestTone,
  requestSetBlockPlugin,
  requestSetBlockMix,
  requestSetBlockBalance,
  requestToggleBlockBypass,
  requestSetBlockBypassMode,
  requestOpenPluginEditor,
  requestAddBlockState,
  requestRecallBlockState,
  requestDeleteBlockState,
} from '../bridge';
import { colors } from './colors';

const bypassModes = [
  { value: 'thru', label: 'Thru' },
  { value: 'mute', label: 'Mute' },
];

export function OptionsPanel() {
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const blocks = useStore((s) => s.blocks);
  const availablePlugins = useStore((s) => s.availablePlugins);

  const block = selectedBlockId
    ? blocks.find((b) => b.id === selectedBlockId)
    : null;

  return (
    <div
      style={{
        width: 280,
        flexShrink: 0,
        boxSizing: 'border-box',
        background: '#0f0d1e',
        borderLeft: `1px solid ${colors.border}`,
        padding: '1rem',
        display: 'flex',
        flexDirection: 'column',
        overflow: 'hidden',
      }}
    >
      <div
        style={{
          fontSize: '1rem',
          fontWeight: 600,
          color: colors.muted,
          letterSpacing: '0.12em',
          textTransform: 'uppercase',
          marginBottom: '1rem',
        }}
      >
        Options
      </div>

      {!block ? (
        <div
          style={{
            fontSize: '1rem',
            color: colors.muted,
            fontStyle: 'italic',
          }}
        >
          Select a block to view options
        </div>
      ) : (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
          <div
            style={{
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'space-between',
            }}
          >
            <div
              style={{
                fontSize: '1rem',
                fontWeight: 700,
                color: block.bypassed ? colors.muted : colors.text,
                letterSpacing: '0.08em',
                textTransform: 'uppercase',
              }}
            >
              {block.name}
            </div>

            {/* Bypass toggle — non-I/O blocks only */}
            {block.type !== 'input' && block.type !== 'output' && (
              <button
                onClick={() => {
                  useStore.getState().setBlockBypassed(block.id, !block.bypassed);
                  requestToggleBlockBypass(block.id);
                }}
                title={block.bypassed ? 'Enable block' : 'Bypass block'}
                style={{
                  position: 'relative',
                  width: 36,
                  height: 20,
                  borderRadius: 10,
                  border: 'none',
                  background: block.bypassed ? colors.border : colors.green,
                  cursor: 'pointer',
                  padding: 0,
                  transition: 'background 0.2s ease',
                }}
              >
                <div
                  style={{
                    position: 'absolute',
                    top: 2,
                    left: block.bypassed ? 2 : 18,
                    width: 16,
                    height: 16,
                    borderRadius: '50%',
                    background: '#ffffff',
                    transition: 'left 0.2s ease',
                  }}
                />
              </button>
            )}
          </div>

          <div style={{ height: 1, background: colors.border }} />

          {/* Input block options */}
          {block.type === 'input' && (
            <OptionRow label="Test Tone">
              <button
                onClick={() => requestToggleTestTone(block.id)}
                title={block.testTone ? 'Disable test tone' : 'Enable test tone'}
                style={{
                  position: 'relative',
                  width: 36,
                  height: 20,
                  borderRadius: 10,
                  border: 'none',
                  background: block.testTone ? colors.green : colors.border,
                  cursor: 'pointer',
                  padding: 0,
                  transition: 'background 0.2s ease',
                }}
              >
                <div
                  style={{
                    position: 'absolute',
                    top: 2,
                    left: block.testTone ? 18 : 2,
                    width: 16,
                    height: 16,
                    borderRadius: '50%',
                    background: '#ffffff',
                    transition: 'left 0.2s ease',
                  }}
                />
              </button>
            </OptionRow>
          )}

          {/* Plugin block options */}
          {block.type === 'plugin' && (
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
                Plugin
              </div>

              {/* Plugin select + options button in one row */}
              <div style={{ display: 'flex', gap: '0.3rem', alignItems: 'stretch' }}>
                <div style={{ flex: 1 }}>
                  <PluginSelect
                    plugins={availablePlugins}
                    selectedId={block.pluginId ?? ''}
                    onSelect={(pluginId) =>
                      requestSetBlockPlugin(block.id, pluginId)
                    }
                  />
                </div>
                {block.pluginId && (
                  <button
                    onClick={() => requestOpenPluginEditor(block.id)}
                    title="Plugin Options"
                    style={{
                      background: 'transparent',
                      border: `1px solid ${colors.border}`,
                      color: colors.muted,
                      padding: '0.3rem',
                      cursor: 'pointer',
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'center',
                    }}
                  >
                    <GearIcon width={16} height={16} />
                  </button>
                )}
              </div>

              {availablePlugins.length === 0 && (
                <div
                  style={{
                    fontSize: '1rem',
                    color: colors.muted,
                    fontStyle: 'italic',
                  }}
                >
                  No plugins found. Scan libraries in Settings.
                </div>
              )}
            </div>
          )}

          {/* Output block — no options */}
          {block.type === 'output' && (
            <div
              style={{
                fontSize: '1rem',
                color: colors.muted,
                fontStyle: 'italic',
              }}
            >
              No options available
            </div>
          )}

          {/* Parameters — for non-I/O blocks */}
          {block.type !== 'input' && block.type !== 'output' && (
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
                          background: '#1a1535',
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
          )}

          {/* States — plugin blocks only, at the bottom */}
          {block.type === 'plugin' && (
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
                  States
                </div>

                <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.4rem' }}>
                  {Array.from({ length: block.numStates ?? 1 }, (_, i) => {
                    const isActive = i === (block.activeStateIndex ?? 0);
                    const isDirty = (block.dirtyStates ?? []).includes(i);
                    return (
                      <StateSquare
                        key={i}
                        index={i}
                        isActive={isActive}
                        isDirty={isDirty}
                        canDelete={(block.numStates ?? 1) > 1}
                        onRecall={() => requestRecallBlockState(block.id, i)}
                        onDelete={() => requestDeleteBlockState(block.id, i)}
                      />
                    );
                  })}

                  {/* Add state button */}
                  {(block.numStates ?? 1) < 16 && (
                    <button
                      onClick={() => requestAddBlockState(block.id)}
                      title="Add new state"
                      style={{
                        width: 32,
                        height: 32,
                        display: 'flex',
                        alignItems: 'center',
                        justifyContent: 'center',
                        background: 'transparent',
                        border: `1px dashed ${colors.border}`,
                        color: colors.muted,
                        cursor: 'pointer',
                        padding: 0,
                      }}
                    >
                      <PlusIcon width={14} height={14} />
                    </button>
                  )}
                </div>
              </div>
            </>
          )}
        </div>
      )}
    </div>
  );
}

function StateSquare({
  index,
  isActive,
  isDirty,
  canDelete,
  onRecall,
  onDelete,
}: {
  index: number;
  isActive: boolean;
  isDirty: boolean;
  canDelete: boolean;
  onRecall: () => void;
  onDelete: () => void;
}) {
  const borderStyle = isActive
    ? `2px solid #ffffff`
    : `1px solid ${colors.border}`;

  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'stretch',
        border: borderStyle,
      }}
    >
      <button
        onClick={() => {
          if (!isActive) onRecall();
        }}
        title={isActive ? `State ${index + 1} (active)` : `Recall state ${index + 1}`}
        style={{
          width: 28,
          height: 28,
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          background: isDirty ? '#ffaa004d' : isActive ? `${colors.secondary}22` : 'transparent',
          border: 'none',
          color: '#ffffff',
          fontSize: '0.9rem',
          fontWeight: 700,
          cursor: 'pointer',
          padding: 0,
        }}
      >
        {index + 1}
      </button>
      {canDelete && (
        <button
          onClick={onDelete}
          title={`Delete State ${index + 1}`}
          style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'center',
            background: 'transparent',
            border: 'none',
            borderLeft: `1px solid ${colors.border}`,
            color: colors.muted,
            padding: '0 0.2rem',
            cursor: 'pointer',
          }}
        >
          <Cross2Icon width={10} height={10} />
        </button>
      )}
    </div>
  );
}

function OptionRow({
  label,
  children,
}: {
  label: string;
  children: React.ReactNode;
}) {
  return (
    <div
      style={{
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
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
        {label}
      </span>
      {children}
    </div>
  );
}
