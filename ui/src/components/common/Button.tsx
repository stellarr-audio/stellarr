import styles from './Button.module.css';

interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: 'default' | 'secondary' | 'danger';
  active?: boolean;
  size?: 'default' | 'sm';
}

export function Button({
  variant = 'default',
  active,
  size = 'default',
  className,
  children,
  ...props
}: ButtonProps) {
  const cls = [
    styles.button,
    variant !== 'default' && styles[variant],
    active && styles.active,
    size !== 'default' && styles[size],
    className,
  ]
    .filter(Boolean)
    .join(' ');
  return (
    <button className={cls} {...props}>
      {children}
    </button>
  );
}
