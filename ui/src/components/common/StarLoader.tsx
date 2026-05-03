import styles from './StarLoader.module.css';

interface Props {
  size?: number;
  className?: string;
  'data-testid'?: string;
  'aria-label'?: string;
}

export function StarLoader({
  size = 16,
  className,
  'data-testid': dataTestId,
  'aria-label': ariaLabel,
}: Props) {
  // Dot diameter scales with size so the constellation reads consistently
  // at any container size (16, 24, 32, etc).
  const dotSize = Math.max(2, Math.round(size * 0.25));

  return (
    <span
      className={`${styles.loader} ${className ?? ''}`}
      style={{
        width: size,
        height: size,
        ['--dot-size' as string]: `${dotSize}px`,
      }}
      role="img"
      aria-label={ariaLabel ?? 'Loading'}
      data-testid={dataTestId}
    >
      <span className={styles.dot} />
      <span className={`${styles.dot} ${styles.orchid}`} />
      <span className={styles.dot} />
    </span>
  );
}
