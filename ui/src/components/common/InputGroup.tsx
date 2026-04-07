import styles from './InputGroup.module.css';

interface InputGroupProps {
  children: React.ReactNode;
}

export function InputGroup({ children }: InputGroupProps) {
  return <div className={styles.group}>{children}</div>;
}

export function InputGroupLabel({ children }: { children: React.ReactNode }) {
  return <span className={styles.label}>{children}</span>;
}
