import styles from './InputGroup.module.css';

interface InputGroupProps {
  children: React.ReactNode;
  /** Compact height (24px) to match IconButton size="sm" and Input size="sm". */
  size?: 'default' | 'sm';
  className?: string;
}

export function InputGroup({ children, size = 'default', className }: InputGroupProps) {
  const cls = [styles.group, size !== 'default' && styles[size], className]
    .filter(Boolean)
    .join(' ');
  return <div className={cls}>{children}</div>;
}

export function InputGroupLabel({ children }: { children: React.ReactNode }) {
  return <span className={styles.label}>{children}</span>;
}
