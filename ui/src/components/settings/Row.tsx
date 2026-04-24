import type { ReactNode } from 'react';
import styles from './Row.module.css';

interface Props {
  info: ReactNode;
  actions?: ReactNode;
}

export function Row({ info, actions }: Props) {
  return (
    <div className={styles.row}>
      <div className={styles.info}>{info}</div>
      {actions !== undefined && <div className={styles.actions}>{actions}</div>}
    </div>
  );
}
