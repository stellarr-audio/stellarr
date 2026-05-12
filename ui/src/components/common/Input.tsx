import { forwardRef } from 'react';
import styles from './Input.module.css';

interface InputProps extends Omit<React.InputHTMLAttributes<HTMLInputElement>, 'size'> {
  /** When true, strips border/radius for use inside an InputGroup. */
  inGroup?: boolean;
  /** Compact height (24px) to line up with IconButton size="sm". */
  size?: 'default' | 'sm';
  /** Render typed content in JetBrains Mono — for numeric-content inputs. */
  mono?: boolean;
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ className, inGroup, size = 'default', mono, ...props }, ref) => {
    const cls = [
      styles.input,
      inGroup && styles.inGroup,
      size !== 'default' && styles[size],
      mono && styles.mono,
      className,
    ]
      .filter(Boolean)
      .join(' ');
    return <input ref={ref} className={cls} {...props} />;
  },
);
