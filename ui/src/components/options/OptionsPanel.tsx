import { useStore } from '../../store';
import { OptionRow } from '../common/OptionRow';
import { Slider } from '../common/Slider';
import { ToggleSwitch } from '../common/ToggleSwitch';
import { PluginSection } from './PluginSection';
import { ParametersSection } from './ParametersSection';
import { StatesSection } from './StatesSection';
import {
  requestToggleTestTone,
  requestToggleBlockBypass,
  requestSetBlockLevel,
} from '../../bridge';
import { colors } from '../common/colors';

export function OptionsPanel() {
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const blocks = useStore((s) => s.blocks);
  const availablePlugins = useStore((s) => s.availablePlugins);

  const block = selectedBlockId ? blocks.find((b) => b.id === selectedBlockId) : null;

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
              Active
            </div>

            {/* Bypass toggle — non-I/O blocks only */}
            {block.type !== 'input' && block.type !== 'output' && (
              <ToggleSwitch
                enabled={!block.bypassed}
                onToggle={() => {
                  useStore.getState().setBlockBypassed(block.id, !block.bypassed);
                  requestToggleBlockBypass(block.id);
                }}
                title={block.bypassed ? 'Enable block' : 'Bypass block'}
              />
            )}
          </div>

          <div style={{ height: 1, background: colors.border }} />

          {/* Input block options */}
          {block.type === 'input' && (
            <OptionRow label="Test Tone">
              <ToggleSwitch
                enabled={block.testTone ?? false}
                onToggle={() => requestToggleTestTone(block.id)}
                title={block.testTone ? 'Disable test tone' : 'Enable test tone'}
              />
            </OptionRow>
          )}

          {/* Plugin block — plugin select */}
          {block.type === 'plugin' && (
            <PluginSection block={block} availablePlugins={availablePlugins} />
          )}

          {/* Level — for I/O blocks (plugin blocks get it via ParametersSection) */}
          {(block.type === 'input' || block.type === 'output') && (
            <>
              <div style={{ height: 1, background: colors.border }} />
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
            </>
          )}

          {/* Parameters — for non-I/O blocks */}
          {block.type !== 'input' && block.type !== 'output' && <ParametersSection block={block} />}

          {/* States — plugin blocks only */}
          {block.type === 'plugin' && <StatesSection block={block} />}
        </div>
      )}
    </div>
  );
}
