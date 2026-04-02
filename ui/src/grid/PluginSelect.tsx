import { useState, useMemo } from 'react';
import { Select } from 'radix-ui';
import { ChevronDownIcon, ChevronUpIcon } from '@radix-ui/react-icons';
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
  const [search, setSearch] = useState('');

  const selected = plugins.find((p) => p.id === selectedId);

  const filtered = useMemo(
    () =>
      search
        ? plugins.filter(
            (p) =>
              p.name.toLowerCase().includes(search.toLowerCase()) ||
              p.manufacturer.toLowerCase().includes(search.toLowerCase()),
          )
        : plugins,
    [plugins, search],
  );

  return (
    <Select.Root value={selectedId} onValueChange={onSelect}>
      <Select.Trigger
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
          outline: 'none',
        }}
      >
        <Select.Value placeholder="Select a plugin..." />
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
            maxHeight: 240,
            overflow: 'auto',
            width: 'var(--radix-select-trigger-width)',
            zIndex: 20,
          }}
        >
          <Select.ScrollUpButton
            style={{
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              height: 20,
              color: colors.muted,
            }}
          >
            <ChevronUpIcon />
          </Select.ScrollUpButton>

          <Select.Viewport>
            {/* Search — using a div so it doesn't interfere with Select keyboard nav */}
            <div style={{ padding: '0.25rem 0.5rem', borderBottom: `1px solid ${colors.border}` }}>
              <input
                type="text"
                placeholder="Search..."
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                onClick={(e) => e.stopPropagation()}
                onKeyDown={(e) => e.stopPropagation()}
                style={{
                  background: colors.cell,
                  color: colors.text,
                  border: 'none',
                  width: '100%',
                  padding: '0.25rem 0',
                  fontSize: '1rem',
                  outline: 'none',
                }}
              />
            </div>

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
              filtered.map((p) => {
                const formatLabel =
                  p.format === 'AudioUnit'
                    ? 'AU'
                    : p.format === 'VST3'
                      ? 'VST3'
                      : p.format;
                const formatColor = formatColors[p.format] ?? colors.muted;

                return (
                  <Select.Item
                    key={p.id}
                    value={p.id}
                    style={{
                      display: 'flex',
                      alignItems: 'center',
                      justifyContent: 'space-between',
                      padding: '0.35rem 0.5rem',
                      cursor: 'pointer',
                      outline: 'none',
                      fontSize: '1rem',
                      color: colors.text,
                    }}
                    onMouseEnter={(e) => {
                      e.currentTarget.style.background = `${colors.border}88`;
                    }}
                    onMouseLeave={(e) => {
                      e.currentTarget.style.background = 'transparent';
                    }}
                  >
                    <div>
                      <Select.ItemText>{p.name}</Select.ItemText>
                      <div
                        style={{
                          fontSize: '0.85rem',
                          color: colors.muted,
                          marginTop: '0.1rem',
                        }}
                      >
                        {p.manufacturer}
                      </div>
                    </div>
                    <span
                      style={{
                        fontSize: '0.85rem',
                        fontWeight: 700,
                        color: formatColor,
                        border: `1px solid ${formatColor}55`,
                        padding: '0.1rem 0.25rem',
                        letterSpacing: '0.06em',
                        textTransform: 'uppercase',
                        flexShrink: 0,
                        marginLeft: '0.5rem',
                      }}
                    >
                      {formatLabel}
                    </span>
                  </Select.Item>
                );
              })
            )}
          </Select.Viewport>

          <Select.ScrollDownButton
            style={{
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              height: 20,
              color: colors.muted,
            }}
          >
            <ChevronDownIcon />
          </Select.ScrollDownButton>
        </Select.Content>
      </Select.Portal>
    </Select.Root>
  );
}
