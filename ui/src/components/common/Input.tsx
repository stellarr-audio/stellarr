import { forwardRef } from 'react';
import styles from './Input.module.css';

interface InputProps extends Omit<React.InputHTMLAttributes<HTMLInputElement>, 'size'> {
  /** When true, strips border/radius for use inside an InputGroup. */
  inGroup?: boolean;
  /** Compact height (24px) to line up with IconButton size="sm". */
  size?: 'default' | 'sm';
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ className, inGroup, size = 'default', ...props }, ref) => {
    const cls = [
      styles.input,
      inGroup && styles.inGroup,
      size !== 'default' && styles[size],
      className,
    ]
      .filter(Boolean)
      .join(' ');
    return <input ref={ref} className={cls} {...props} />;
  },
);
