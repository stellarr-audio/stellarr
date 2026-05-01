import { useState, type ReactNode } from 'react';
import styles from './ShapingDisclosure.module.css';

interface Props {
  children: ReactNode;
  defaultOpen?: boolean;
}

export function ShapingDisclosure({ children, defaultOpen = false }: Props) {
  const [open, setOpen] = useState(defaultOpen);
  return (
    <div className={styles.wrap}>
      <button
        type="button"
        onClick={() => setOpen((v) => !v)}
        className={`${styles.button} ${open ? styles.open : ''}`}
        aria-expanded={open}
      >
        <span className={styles.caret}>▶</span>
        Shaping
      </button>
      {open && <div className={styles.body}>{children}</div>}
    </div>
  );
}
