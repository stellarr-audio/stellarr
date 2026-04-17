import type { ReactNode } from 'react';
import styles from './IconButton.module.css';

interface IconButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  icon: ReactNode;
  /** When true, strips border/radius for use inside an InputGroup. */
  inGroup?: boolean;
  size?: 'default' | 'sm';
  variant?: 'default' | 'danger';
}

export function IconButton({
  icon,
  className,
  inGroup,
  size = 'default',
  variant = 'default',
  ...props
}: IconButtonProps) {
  const cls = [
    styles.button,
    size !== 'default' && styles[size],
    variant !== 'default' && styles[variant],
    inGroup && styles.inGroup,
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
