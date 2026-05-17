import styles from './Button.module.css';

interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
  /**
   * `default` — neutral border, orchid hover (used for plain actions).
   * `secondary` — neutral border, amber hover (used for cancel-style actions).
   * `danger` — rose text, rose hover (used for destructive actions).
   * `primary` — solid orchid fill, white text (used for primary CTAs / form submits).
   */
  variant?: 'default' | 'secondary' | 'danger' | 'primary';
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
