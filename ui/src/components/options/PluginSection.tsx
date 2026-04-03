import { GearIcon } from '@radix-ui/react-icons';
import { PluginSelect } from './PluginSelect';
import { requestSetBlockPlugin, requestOpenPluginEditor } from '../../bridge';
import styles from './PluginSection.module.css';
import type { GridBlock, PluginInfo } from '../../store';

interface Props {
  block: GridBlock;
  availablePlugins: PluginInfo[];
}

export function PluginSection({ block, availablePlugins }: Props) {
  return (
    <div className={styles.container}>
      <div className={styles.sectionTitle}>Plugin</div>

      <div className={styles.row}>
        <div className={styles.selectWrapper}>
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
            className={styles.optionsButton}
          >
            <GearIcon width={16} height={16} />
          </button>
        )}
      </div>

      {availablePlugins.length === 0 && (
        <div className={styles.emptyMessage}>No plugins found. Scan libraries in Settings.</div>
      )}
    </div>
  );
}
