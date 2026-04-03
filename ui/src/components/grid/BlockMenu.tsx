import { DropdownMenu } from 'radix-ui';
import { colors } from '../common/colors';

const menuItems = [
  { type: 'input', label: 'Input' },
  { type: 'output', label: 'Output' },
  { type: 'plugin', label: 'Plugin' },
];

interface Props {
  open: boolean;
  onSelect: (type: string) => void;
  onClose: () => void;
  children: React.ReactNode;
}

export function BlockMenu({ open, onSelect, onClose, children }: Props) {
  return (
    <DropdownMenu.Root open={open} onOpenChange={(o) => { if (!o) onClose(); }}>
      <DropdownMenu.Trigger asChild>
        {children}
      </DropdownMenu.Trigger>

      <DropdownMenu.Portal>
        <DropdownMenu.Content
          sideOffset={4}
          style={{
            background: '#1a1535',
            border: `1px solid ${colors.border}`,
            padding: '0.25rem 0',
            minWidth: 120,
            zIndex: 10,
            
          }}
        >
          {menuItems.map((item) => (
            <DropdownMenu.Item
              key={item.type}
              onSelect={() => onSelect(item.type)}
              style={{
                padding: '0.35rem 0.75rem',
                fontSize: '1rem',
                fontWeight: 600,
                letterSpacing: '0.06em',
                textTransform: 'uppercase',
                color: colors.text,
                cursor: 'pointer',
                outline: 'none',
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.background = colors.border;
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.background = 'transparent';
              }}
            >
              {item.label}
            </DropdownMenu.Item>
          ))}
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}
