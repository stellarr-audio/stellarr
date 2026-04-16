import { useStore } from '../../store';
import { ToggleSwitch } from '../common/ToggleSwitch';
import { Input } from '../common/Input';
import { InputGroup, InputGroupLabel } from '../common/InputGroup';
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
        <ToggleSwitch
          enabled={enabled}
          onToggle={toggle}
          title={enabled ? 'Disable target loudness' : 'Enable target loudness'}
        />
      </div>
      {enabled && (
        <InputGroup>
          <Input
            inGroup
            type="number"
            value={target ?? -18}
            onChange={onChange}
            min={-30}
            max={-6}
            step={0.5}
          />
          <InputGroupLabel>LUFS</InputGroupLabel>
        </InputGroup>
      )}
    </div>
  );
}
