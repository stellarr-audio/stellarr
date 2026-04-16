import type { ReactNode } from 'react';
import styles from './IconButton.module.css';

interface IconButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  icon: ReactNode;
  /** When true, strips border/radius for use inside an InputGroup. */
  inGroup?: boolean;
}

export function IconButton({ icon, className, inGroup, ...props }: IconButtonProps) {
  const cls = [styles.button, inGroup && styles.inGroup, className].filter(Boolean).join(' ');
  return (
    <button className={cls} {...props}>
      {icon}
    </button>
  );
}
