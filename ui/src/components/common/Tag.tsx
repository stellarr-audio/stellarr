import styles from './Tag.module.css';

interface TagProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  active?: boolean;
}

export function Tag({ active, className, children, ...props }: TagProps) {
  const cls = [styles.tag, active && styles.active, className].filter(Boolean).join(' ');
  return (
    <button type="button" className={cls} {...props}>
      {children}
    </button>
  );
}
