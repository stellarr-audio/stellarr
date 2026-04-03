import { useStore } from '../../store';
import {
  requestScanPlugins,
  requestPickScanDirectory,
  requestRemoveScanDirectory,
} from '../../bridge';
import { colors } from '../common/colors';
import { MidiSettings } from './MidiSettings';

export function Settings() {
  const scanDirectories = useStore((s) => s.scanDirectories);
  const availablePlugins = useStore((s) => s.availablePlugins);

  return (
    <div style={{ flex: 1, display: 'flex', overflow: 'hidden' }}>
      <div
        style={{
          flex: 1,
          padding: '2rem',
          overflow: 'auto',
        }}
      >
        {/* Libraries section */}
        <Section title="Libraries">
          <div
            style={{
              display: 'flex',
              flexDirection: 'column',
              gap: '0.5rem',
              marginBottom: '1rem',
            }}
          >
            {scanDirectories.map((dir) => (
              <div
                key={dir.path}
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'space-between',
                  padding: '0.5rem 0.75rem',
                  background: colors.cell,
                  border: `1px solid ${colors.border}`,
                }}
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                  <span
                    style={{
                      fontSize: '1rem',
                      color: colors.text,
                      wordBreak: 'break-all',
                    }}
                  >
                    {dir.path}
                  </span>
                  {dir.isDefault && (
                    <span
                      style={{
                        fontSize: '1rem',
                        fontWeight: 600,
                        color: colors.secondary,
                        letterSpacing: '0.08em',
                        textTransform: 'uppercase',
                        border: `1px solid ${colors.secondary}44`,
                        padding: '0.1rem 0.3rem',
                        flexShrink: 0,
                      }}
                    >
                      System
                    </span>
                  )}
                </div>
                {!dir.isDefault && (
                  <button
                    onClick={() => requestRemoveScanDirectory(dir.path)}
                    style={{
                      background: 'transparent',
                      border: 'none',
                      color: colors.muted,
                      fontSize: '1rem',
                      cursor: 'pointer',
                      padding: '0 0.25rem',
                      flexShrink: 0,
                    }}
                  >
                    x
                  </button>
                )}
              </div>
            ))}
          </div>

          <div style={{ display: 'flex', gap: '0.5rem' }}>
            <ActionButton onClick={requestPickScanDirectory}>Add Directory</ActionButton>
            <ActionButton onClick={requestScanPlugins}>Scan Now</ActionButton>
          </div>
        </Section>

        {/* Discovered plugins section */}
        <Section title={`Discovered Plugins (${availablePlugins.length})`}>
          {availablePlugins.length === 0 ? (
            <div
              style={{
                fontSize: '1rem',
                color: colors.muted,
                fontStyle: 'italic',
              }}
            >
              No plugins found. Add library directories and click "Scan Now".
            </div>
          ) : (
            <div
              style={{
                display: 'flex',
                flexDirection: 'column',
                gap: '0.25rem',
                maxHeight: 300,
                overflow: 'auto',
              }}
            >
              {availablePlugins.map((plugin) => (
                <div
                  key={plugin.id}
                  style={{
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    padding: '0.4rem 0.75rem',
                    background: colors.cell,
                    border: `1px solid ${colors.border}`,
                  }}
                >
                  <div>
                    <div
                      style={{
                        fontSize: '1rem',
                        fontWeight: 600,
                        color: colors.text,
                      }}
                    >
                      {plugin.name}
                    </div>
                    <div
                      style={{
                        fontSize: '1rem',
                        color: colors.muted,
                      }}
                    >
                      {plugin.manufacturer}
                    </div>
                  </div>
                  <span
                    style={{
                      fontSize: '1rem',
                      color: colors.muted,
                      letterSpacing: '0.06em',
                      textTransform: 'uppercase',
                    }}
                  >
                    {plugin.format}
                  </span>
                </div>
              ))}
            </div>
          )}
        </Section>

        {/* MIDI mappings section */}
        <Section title="MIDI">
          <MidiSettings />
        </Section>
      </div>

      {/* App info panel */}
      <div
        style={{
          width: 280,
          flexShrink: 0,
          padding: '1rem',
          boxSizing: 'border-box',
          borderLeft: `1px solid ${colors.border}`,
          display: 'flex',
          flexDirection: 'column',
          justifyContent: 'center',
          alignItems: 'center',
          gap: '1rem',
        }}
      >
        <span
          style={{
            fontSize: '1.2rem',
            fontWeight: 700,
            color: colors.primary,
            letterSpacing: '0.1em',
            textTransform: 'uppercase',
            textShadow: `0 0 12px ${colors.primary}44`,
          }}
        >
          Stellarr
        </span>
        <span
          style={{
            fontSize: '1rem',
            color: colors.muted,
            letterSpacing: '0.06em',
          }}
        >
          v0.1.0
        </span>
        <p
          style={{
            fontSize: '1rem',
            color: colors.muted,
            textAlign: 'center',
            lineHeight: 1.6,
            margin: 0,
            maxWidth: 220,
          }}
        >
          Made by an AI and a human, together. Free and open, forever. For the love of music and
          art.
        </p>
      </div>
    </div>
  );
}

function Section({ title, children }: { title: string; children: React.ReactNode }) {
  return (
    <div style={{ marginBottom: '2rem' }}>
      <h3
        style={{
          fontSize: '1rem',
          fontWeight: 600,
          color: colors.secondary,
          letterSpacing: '0.1em',
          textTransform: 'uppercase',
          margin: '0 0 0.75rem 0',
        }}
      >
        {title}
      </h3>
      {children}
    </div>
  );
}

function ActionButton({ onClick, children }: { onClick: () => void; children: React.ReactNode }) {
  return (
    <button
      onClick={onClick}
      style={{
        background: 'transparent',
        color: colors.text,
        border: `1px solid ${colors.border}`,
        padding: '0.35rem 0.75rem',
        fontSize: '1rem',
        fontWeight: 600,
        letterSpacing: '0.06em',
        textTransform: 'uppercase',
        cursor: 'pointer',
      }}
      onMouseEnter={(e) => {
        e.currentTarget.style.borderColor = colors.muted;
      }}
      onMouseLeave={(e) => {
        e.currentTarget.style.borderColor = colors.border;
      }}
    >
      {children}
    </button>
  );
}
