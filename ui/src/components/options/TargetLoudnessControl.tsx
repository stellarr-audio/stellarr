import { useStore } from '../../store';
import { requestSetTargetLufs } from '../../bridge';
import styles from './TargetLoudnessControl.module.css';

interface Props {
  blockId: string;
}

export function TargetLoudnessControl({ blockId }: Props) {
  const target = useStore((s) => s.targetLufsByBlockId[blockId]);
  const enabled = target != null;

  const toggle = () => {
    if (enabled) requestSetTargetLufs(blockId, null);
    else requestSetTargetLufs(blockId, -18);
  };

  const onChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const value = parseFloat(e.target.value);
    if (!isNaN(value)) requestSetTargetLufs(blockId, value);
  };

  return (
    <div className={styles.section}>
      <div className={styles.header}>
        <span className={styles.label}>Target Loudness</span>
        <button className={styles.toggle} onClick={toggle}>
          {enabled ? 'On' : 'Off'}
        </button>
      </div>
      {enabled && (
        <div className={styles.row}>
          <input
            type="number"
            className={styles.input}
            value={target ?? -18}
            onChange={onChange}
            min={-30}
            max={-6}
            step={0.5}
          />
          <span className={styles.unit}>LUFS</span>
        </div>
      )}
    </div>
  );
}
