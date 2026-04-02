import { useStore } from '../store';
import {
  requestToggleTestTone,
  requestSetBlockPlugin,
  requestOpenPluginEditor,
} from '../bridge';
import { colors } from './colors';

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
        width: 240,
        flexShrink: 0,
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
          fontSize: '0.6rem',
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
            fontSize: '0.7rem',
            color: colors.muted,
            fontStyle: 'italic',
          }}
        >
          Select a block to view options
        </div>
      ) : (
        <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
          <div>
            <div
              style={{
                fontSize: '0.85rem',
                fontWeight: 700,
                color: colors.text,
                letterSpacing: '0.08em',
                textTransform: 'uppercase',
              }}
            >
              {block.name}
            </div>
          </div>

          <div style={{ height: 1, background: colors.border }} />

          {/* Input block options */}
          {block.type === 'input' && (
            <OptionRow label="Test Tone">
              <button
                onClick={() => requestToggleTestTone(block.id)}
                style={{
                  background: block.testTone ? colors.green : 'transparent',
                  color: block.testTone ? '#0d0b1a' : colors.muted,
                  border: `1px solid ${block.testTone ? colors.green : colors.muted}`,
                  padding: '0.25rem 0.6rem',
                  fontSize: '0.6rem',
                  fontWeight: 600,
                  letterSpacing: '0.06em',
                  textTransform: 'uppercase',
                  cursor: 'pointer',
                }}
              >
                {block.testTone ? 'On' : 'Off'}
              </button>
            </OptionRow>
          )}

          {/* VST block options */}
          {block.type === 'vst' && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem' }}>
              <div
                style={{
                  fontSize: '0.6rem',
                  fontWeight: 600,
                  color: colors.secondary,
                  letterSpacing: '0.08em',
                  textTransform: 'uppercase',
                }}
              >
                Plugin
              </div>

              {block.pluginName && (
                <div
                  style={{
                    fontSize: '0.65rem',
                    color: colors.green,
                    fontWeight: 600,
                  }}
                >
                  {block.pluginName}
                </div>
              )}

              <select
                value={block.pluginId ?? ''}
                onChange={(e) => {
                  if (e.target.value)
                    requestSetBlockPlugin(block.id, e.target.value);
                }}
                style={{
                  background: colors.cell,
                  color: colors.text,
                  border: `1px solid ${colors.border}`,
                  padding: '0.35rem 0.5rem',
                  fontSize: '0.6rem',
                  outline: 'none',
                  width: '100%',
                }}
              >
                <option value="">Select a plugin...</option>
                {availablePlugins.map((p) => (
                  <option key={p.id} value={p.id}>
                    {p.name} ({p.manufacturer})
                  </option>
                ))}
              </select>

              {availablePlugins.length === 0 && (
                <div
                  style={{
                    fontSize: '0.55rem',
                    color: colors.muted,
                    fontStyle: 'italic',
                  }}
                >
                  No plugins found. Scan libraries in Settings.
                </div>
              )}

              {block.pluginId && (
                <button
                  onClick={() => requestOpenPluginEditor(block.id)}
                  style={{
                    background: 'transparent',
                    color: colors.secondary,
                    border: `1px solid ${colors.secondary}`,
                    padding: '0.3rem 0.6rem',
                    fontSize: '0.6rem',
                    fontWeight: 600,
                    letterSpacing: '0.06em',
                    textTransform: 'uppercase',
                    cursor: 'pointer',
                    marginTop: '0.25rem',
                  }}
                >
                  Plugin Options
                </button>
              )}
            </div>
          )}

          {/* Output block — no options */}
          {block.type === 'output' && (
            <div
              style={{
                fontSize: '0.65rem',
                color: colors.muted,
                fontStyle: 'italic',
              }}
            >
              No options available
            </div>
          )}
        </div>
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
          fontSize: '0.65rem',
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
