import styles from './ToggleSwitch.module.css';

interface Props {
  enabled: boolean;
  onToggle: () => void;
  title?: string;
}

export function ToggleSwitch({ enabled, onToggle, title }: Props) {
  return (
    <button
      onClick={onToggle}
      title={title}
      className={`${styles.toggle} ${enabled ? styles.enabled : ''}`}
    >
      <div className={styles.thumb} />
    </button>
  );
}
