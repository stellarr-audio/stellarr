interface Props {
  size?: number;
  style?: React.CSSProperties;
}

export function Logo({ size = 20, style }: Props) {
  return <img src="/logo.svg" width={size} height={size} alt="Stellarr" style={style} />;
}
