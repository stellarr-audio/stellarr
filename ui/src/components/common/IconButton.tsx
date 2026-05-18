import type { ReactNode } from 'react';
import styles from './IconButton.module.css';

interface IconButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  icon: ReactNode;
  /** When true, strips border/radius for use inside an InputGroup. */
  inGroup?: boolean;
  size?: 'default' | 'sm';
  /**
   * `default` — neutral border, amber hover (standard icon action).
   * `danger` — rose glyph + rose hover (destructive / close affordance).
   * `primary` — solid orchid fill + white glyph (icon equivalent of
   * `<Button variant="primary">` — for primary-CTA-style icon buttons).
   */
  variant?: 'default' | 'danger' | 'primary';
  /** When true, applies the orchid "currently selected" tint (different from `variant="primary"`'s solid fill). */
  active?: boolean;
}

export function IconButton({
  icon,
  className,
  inGroup,
  size = 'default',
  variant = 'default',
  active,
  ...props
}: IconButtonProps) {
  const cls = [
    styles.button,
    size !== 'default' && styles[size],
    variant !== 'default' && styles[variant],
    inGroup && styles.inGroup,
    active && styles.active,
    className,
  ]
    .filter(Boolean)
    .join(' ');
  return (
    <button className={cls} {...props}>
      {icon}
    </button>
  );
}
