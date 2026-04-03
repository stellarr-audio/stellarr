import styles from './Logo.module.css';

interface Props {
  size?: number;
  className?: string;
  style?: React.CSSProperties;
}

export function Logo({ size = 20, className, style }: Props) {
  return (
    <img
      src="/logo.svg"
      width={size}
      height={size}
      alt="Stellarr"
      className={`${styles.logo} ${className ?? ''}`}
      style={style}
    />
  );
}
