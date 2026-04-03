import { useStore } from '../../store';
import { OptionRow } from '../common/OptionRow';
import { PluginSection } from './PluginSection';
import { ParametersSection } from './ParametersSection';
import { StatesSection } from './StatesSection';
import { requestToggleTestTone, requestToggleBlockBypass } from '../../bridge';
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

          {/* Plugin block — plugin select */}
          {block.type === 'plugin' && (
            <PluginSection block={block} availablePlugins={availablePlugins} />
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
          {block.type !== 'input' && block.type !== 'output' && <ParametersSection block={block} />}

          {/* States — plugin blocks only */}
          {block.type === 'plugin' && <StatesSection block={block} />}
        </div>
      )}
    </div>
  );
}
