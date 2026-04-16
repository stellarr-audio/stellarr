import { GearIcon } from '@radix-ui/react-icons';
import { IconButton } from '../common/IconButton';
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

      <div className={styles.inputGroup}>
        <div className={styles.selectWrapper}>
          <PluginSelect
            plugins={availablePlugins}
            selectedId={block.pluginId ?? ''}
            onSelect={(pluginId) => requestSetBlockPlugin(block.id, pluginId)}
          />
        </div>
        {block.pluginId && (
          <IconButton
            inGroup
            icon={<GearIcon width={16} height={16} />}
            onClick={() => requestOpenPluginEditor(block.id)}
            title="Plugin Options"
          />
        )}
      </div>

      {availablePlugins.length === 0 && (
        <div className={styles.emptyMessage}>No plugins found. Scan libraries in Settings.</div>
      )}
    </div>
  );
}
