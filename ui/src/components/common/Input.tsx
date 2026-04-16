import { forwardRef } from 'react';
import styles from './Input.module.css';

interface InputProps extends React.InputHTMLAttributes<HTMLInputElement> {
  /** When true, strips border/radius for use inside an InputGroup. */
  inGroup?: boolean;
}

export const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ className, inGroup, ...props }, ref) => {
    const cls = [styles.input, inGroup && styles.inGroup, className].filter(Boolean).join(' ');
    return <input ref={ref} className={cls} {...props} />;
  },
);
