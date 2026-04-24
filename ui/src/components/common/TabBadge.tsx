import type { Badge } from '../../store';
import styles from './TabBadge.module.css';

interface Props {
  badge: Badge | undefined;
}

export function TabBadge({ badge }: Props) {
  if (!badge) return null;
  return <span className={styles.dot} data-severity={badge.severity} aria-hidden="true" />;
}
