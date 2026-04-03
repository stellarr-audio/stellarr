interface Props {
  size?: number;
  color?: string;
  style?: React.CSSProperties;
}

export function Logo({ size = 20, color = 'currentColor', style }: Props) {
  return (
    <svg
      width={size}
      height={size}
      viewBox="0 0 24 24"
      fill="none"
      style={style}
    >
      <path
        d="M 1 12 Q 6 9, 9 12 Q 10.5 13.5, 12 12 Q 13.5 10.5, 15 12 Q 18 15, 23 12"
        stroke={color}
        strokeWidth="2"
        strokeLinecap="round"
      />
      <path
        d="M 12 1 Q 10.5 7, 12 9"
        stroke={color}
        strokeWidth="1.5"
        strokeLinecap="round"
      />
      <path
        d="M 12 15 Q 13.5 17, 12 23"
        stroke={color}
        strokeWidth="1.5"
        strokeLinecap="round"
      />
      <circle cx="12" cy="12" r="1.5" fill={color} />
    </svg>
  );
}
