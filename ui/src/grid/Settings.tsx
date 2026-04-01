import { useStore } from '../store';
import {
  requestScanPlugins,
  requestPickScanDirectory,
  requestRemoveScanDirectory,
} from '../bridge';
import { colors } from './colors';

export function Settings() {
  const scanDirectories = useStore((s) => s.scanDirectories);
  const availablePlugins = useStore((s) => s.availablePlugins);

  return (
    <div
      style={{
        flex: 1,
        padding: '2rem',
        overflow: 'auto',
      }}
    >
      <h2
        style={{
          fontSize: '0.8rem',
          fontWeight: 700,
          color: colors.text,
          letterSpacing: '0.1em',
          textTransform: 'uppercase',
          margin: '0 0 1.5rem 0',
        }}
      >
        Settings
      </h2>

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
                    fontSize: '0.65rem',
                    color: colors.text,
                    wordBreak: 'break-all',
                  }}
                >
                  {dir.path}
                </span>
                {dir.isDefault && (
                  <span
                    style={{
                      fontSize: '0.5rem',
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
                    fontSize: '0.7rem',
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
          <ActionButton onClick={requestPickScanDirectory}>
            Add Directory
          </ActionButton>
          <ActionButton onClick={requestScanPlugins}>
            Scan Now
          </ActionButton>
        </div>
      </Section>

      {/* Discovered plugins section */}
      <Section title={`Discovered Plugins (${availablePlugins.length})`}>
        {availablePlugins.length === 0 ? (
          <div
            style={{
              fontSize: '0.65rem',
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
                      fontSize: '0.65rem',
                      fontWeight: 600,
                      color: colors.text,
                    }}
                  >
                    {plugin.name}
                  </div>
                  <div
                    style={{
                      fontSize: '0.5rem',
                      color: colors.muted,
                    }}
                  >
                    {plugin.manufacturer}
                  </div>
                </div>
                <span
                  style={{
                    fontSize: '0.5rem',
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
    </div>
  );
}

function Section({
  title,
  children,
}: {
  title: string;
  children: React.ReactNode;
}) {
  return (
    <div style={{ marginBottom: '2rem' }}>
      <h3
        style={{
          fontSize: '0.65rem',
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

function ActionButton({
  onClick,
  children,
}: {
  onClick: () => void;
  children: React.ReactNode;
}) {
  return (
    <button
      onClick={onClick}
      style={{
        background: 'transparent',
        color: colors.text,
        border: `1px solid ${colors.border}`,
        padding: '0.35rem 0.75rem',
        fontSize: '0.6rem',
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
