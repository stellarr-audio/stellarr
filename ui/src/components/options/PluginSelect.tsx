import { useState, useMemo } from 'react';
import { Select } from 'radix-ui';
import { ChevronDownIcon, ChevronUpIcon } from '@radix-ui/react-icons';
import type { PluginInfo } from '../../store';
import styles from './PluginSelect.module.css';

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

  const filtered = useMemo(() => {
    const sorted = plugins.toSorted((a, b) => a.name.localeCompare(b.name));
    return search
      ? sorted.filter(
          (p) =>
            p.name.toLowerCase().includes(search.toLowerCase()) ||
            p.manufacturer.toLowerCase().includes(search.toLowerCase()),
        )
      : sorted;
  }, [plugins, search]);

  return (
    <Select.Root value={selectedId} onValueChange={onSelect}>
      <Select.Trigger
        className={`${styles.trigger} ${selected ? styles.triggerSelected : styles.triggerPlaceholder}`}
      >
        <Select.Value placeholder="Select a plugin..." />
        <Select.Icon>
          <ChevronDownIcon />
        </Select.Icon>
      </Select.Trigger>

      <Select.Portal>
        <Select.Content position="popper" sideOffset={4} className={styles.content}>
          <Select.ScrollUpButton className={styles.scrollButton}>
            <ChevronUpIcon />
          </Select.ScrollUpButton>

          <Select.Viewport>
            {/* Search — using a div so it doesn't interfere with Select keyboard nav */}
            <div className={styles.searchWrapper}>
              <input
                type="text"
                placeholder="Search..."
                value={search}
                onChange={(e) => setSearch(e.target.value)}
                onClick={(e) => e.stopPropagation()}
                onKeyDown={(e) => e.stopPropagation()}
                className={styles.searchInput}
              />
            </div>

            {filtered.length === 0 ? (
              <div className={styles.emptyMessage}>No plugins found</div>
            ) : (
              filtered.map((p) => {
                const formatLabel =
                  p.format === 'AudioUnit' ? 'AU' : p.format === 'VST3' ? 'VST3' : p.format;
                const formatColor = formatColors[p.format] ?? '#5a5478';

                return (
                  <Select.Item key={p.id} value={p.id} className={styles.item}>
                    <div>
                      <Select.ItemText>{p.name}</Select.ItemText>
                      <div className={styles.itemManufacturer}>{p.manufacturer}</div>
                    </div>
                    <span
                      className={styles.formatBadge}
                      style={{
                        color: formatColor,
                        border: `1px solid ${formatColor}55`,
                      }}
                    >
                      {formatLabel}
                    </span>
                  </Select.Item>
                );
              })
            )}
          </Select.Viewport>

          <Select.ScrollDownButton className={styles.scrollButton}>
            <ChevronDownIcon />
          </Select.ScrollDownButton>
        </Select.Content>
      </Select.Portal>
    </Select.Root>
  );
}
