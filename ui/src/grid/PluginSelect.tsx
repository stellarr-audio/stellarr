import { useState, useRef, useEffect } from 'react';
import type { PluginInfo } from '../store';
import { colors } from './colors';

const formatColors: Record<string, string> = {
  VST3: '#00b4ff',
  AudioUnit: '#ff8800',
  VST: '#00ff9d',
};

interface Props {
  plugins: PluginInfo[];
  selectedId: string;
  onSelect: (pluginId: string) => void;
}

export function PluginSelect({ plugins, selectedId, onSelect }: Props) {
  const [open, setOpen] = useState(false);
  const [filter, setFilter] = useState('');
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const handler = (e: MouseEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node))
        setOpen(false);
    };
    document.addEventListener('mousedown', handler);
    return () => document.removeEventListener('mousedown', handler);
  }, [open]);

  const selected = plugins.find((p) => p.id === selectedId);

  const filtered = filter
    ? plugins.filter(
        (p) =>
          p.name.toLowerCase().includes(filter.toLowerCase()) ||
          p.manufacturer.toLowerCase().includes(filter.toLowerCase()),
      )
    : plugins;

  return (
    <div ref={ref} style={{ position: 'relative' }}>
      {/* Trigger button */}
      <button
        onClick={() => setOpen(!open)}
        style={{
          width: '100%',
          textAlign: 'left',
          background: colors.cell,
          color: selected ? colors.text : colors.muted,
          border: `1px solid ${colors.border}`,
          padding: '0.35rem 0.5rem',
          fontSize: '1rem',
          cursor: 'pointer',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'space-between',
          gap: '0.25rem',
        }}
      >
        <span
          style={{
            overflow: 'hidden',
            textOverflow: 'ellipsis',
            whiteSpace: 'nowrap',
          }}
        >
          {selected ? selected.name : 'Select a plugin...'}
        </span>
        <span style={{ fontSize: '1rem', color: colors.muted, flexShrink: 0 }}>
          {open ? '\u25B2' : '\u25BC'}
        </span>
      </button>

      {/* Dropdown */}
      {open && (
        <div
          style={{
            position: 'absolute',
            top: '100%',
            left: 0,
            right: 0,
            marginTop: 2,
            background: '#1a1535',
            border: `1px solid ${colors.border}`,
            zIndex: 20,
            maxHeight: 240,
            display: 'flex',
            flexDirection: 'column',
          }}
        >
          {/* Search */}
          <input
            type="text"
            placeholder="Search..."
            value={filter}
            onChange={(e) => setFilter(e.target.value)}
            autoFocus
            style={{
              background: colors.cell,
              color: colors.text,
              border: 'none',
              borderBottom: `1px solid ${colors.border}`,
              padding: '0.35rem 0.5rem',
              fontSize: '1rem',
              outline: 'none',
            }}
          />

          {/* List */}
          <div style={{ overflow: 'auto', flex: 1 }}>
            {filtered.length === 0 ? (
              <div
                style={{
                  padding: '0.5rem',
                  fontSize: '1rem',
                  color: colors.muted,
                  fontStyle: 'italic',
                }}
              >
                No plugins found
              </div>
            ) : (
              filtered.map((p) => (
                <div
                  key={p.id}
                  onClick={() => {
                    onSelect(p.id);
                    setOpen(false);
                    setFilter('');
                  }}
                  style={{
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'space-between',
                    padding: '0.35rem 0.5rem',
                    cursor: 'pointer',
                    background:
                      p.id === selectedId ? colors.border : 'transparent',
                  }}
                  onMouseEnter={(e) => {
                    if (p.id !== selectedId)
                      e.currentTarget.style.background = `${colors.border}88`;
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.background =
                      p.id === selectedId ? colors.border : 'transparent';
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
                      {p.name}
                    </div>
                    <div
                      style={{
                        fontSize: '1rem',
                        color: colors.muted,
                        marginTop: '0.1rem',
                      }}
                    >
                      {p.manufacturer}
                    </div>
                  </div>
                  <FormatTag format={p.format} />
                </div>
              ))
            )}
          </div>
        </div>
      )}
    </div>
  );
}

function FormatTag({ format }: { format: string }) {
  const label =
    format === 'AudioUnit' ? 'AU' : format === 'VST3' ? 'VST3' : format;
  const color = formatColors[format] ?? colors.muted;

  return (
    <span
      style={{
        fontSize: '1rem',
        fontWeight: 700,
        color,
        border: `1px solid ${color}55`,
        padding: '0.1rem 0.25rem',
        letterSpacing: '0.06em',
        textTransform: 'uppercase',
        flexShrink: 0,
      }}
    >
      {label}
    </span>
  );
}
