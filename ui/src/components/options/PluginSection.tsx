import { GearIcon } from '@radix-ui/react-icons';
import { PluginSelect } from './PluginSelect';
import { requestSetBlockPlugin, requestOpenPluginEditor } from '../../bridge';
import { colors } from '../common/colors';
import type { GridBlock, PluginInfo } from '../../store';

interface Props {
  block: GridBlock;
  availablePlugins: PluginInfo[];
}

export function PluginSection({ block, availablePlugins }: Props) {
  return (
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

      <div style={{ display: 'flex', gap: '0.3rem', alignItems: 'stretch' }}>
        <div style={{ flex: 1 }}>
          <PluginSelect
            plugins={availablePlugins}
            selectedId={block.pluginId ?? ''}
            onSelect={(pluginId) => requestSetBlockPlugin(block.id, pluginId)}
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
  );
}
