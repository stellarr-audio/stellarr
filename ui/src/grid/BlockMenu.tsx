import { useEffect, useRef } from 'react';
import { colors } from './colors';

export interface MenuItem {
  type: string;
  label: string;
}

const menuItems: MenuItem[] = [
  { type: 'input', label: 'Input' },
  { type: 'output', label: 'Output' },
  { type: 'vst', label: 'VST Plugin' },
];

interface Props {
  x: number;
  y: number;
  anchor?: 'top' | 'bottom';
  onSelect: (type: string) => void;
  onClose: () => void;
}

export function BlockMenu({ x, y, anchor = 'top', onSelect, onClose }: Props) {
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const handler = (e: MouseEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node))
        onClose();
    };
    document.addEventListener('mousedown', handler);
    return () => document.removeEventListener('mousedown', handler);
  }, [onClose]);

  return (
    <div
      ref={ref}
      style={{
        position: 'absolute',
        left: x,
        ...(anchor === 'bottom' ? { bottom: `calc(100% - ${y}px + 4px)` } : { top: y }),
        background: '#1a1535',
        border: `1px solid ${colors.border}`,
        padding: '0.25rem 0',
        zIndex: 10,
        minWidth: 120,
      }}
    >
      {menuItems.map((item) => (
        <div
          key={item.type}
          onClick={() => onSelect(item.type)}
          style={{
            padding: '0.35rem 0.75rem',
            fontSize: '0.7rem',
            fontWeight: 600,
            letterSpacing: '0.06em',
            textTransform: 'uppercase',
            color: colors.text,
            cursor: 'pointer',
          }}
          onMouseEnter={(e) => {
            e.currentTarget.style.background = colors.border;
          }}
          onMouseLeave={(e) => {
            e.currentTarget.style.background = 'transparent';
          }}
        >
          {item.label}
        </div>
      ))}
    </div>
  );
}
