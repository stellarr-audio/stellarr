import {
  createContext,
  forwardRef,
  useContext,
  useRef,
  type KeyboardEvent,
  type ReactNode,
} from 'react';
import styles from './Tablist.module.css';

interface TablistContextValue {
  value: string;
  onChange: (id: string) => void;
  register: (id: string, el: HTMLButtonElement | null) => void;
  focusSibling: (id: string, direction: 1 | -1) => void;
  accent: 'primary' | 'secondary';
}

const TablistContext = createContext<TablistContextValue | null>(null);

interface TablistProps {
  value: string;
  onChange: (id: string) => void;
  children: ReactNode;
  stretch?: boolean;
  /** Active-tab accent. Primary (orchid) for nav; secondary (amber) for inline mode switches. */
  accent?: 'primary' | 'secondary';
  className?: string;
  'aria-label'?: string;
}

export function Tablist({
  value,
  onChange,
  children,
  stretch,
  accent = 'primary',
  className,
  'aria-label': ariaLabel,
}: TablistProps) {
  const orderRef = useRef<string[]>([]);
  const elsRef = useRef<Map<string, HTMLButtonElement>>(new Map());

  const register = (id: string, el: HTMLButtonElement | null) => {
    if (el) {
      elsRef.current.set(id, el);
      if (!orderRef.current.includes(id)) orderRef.current.push(id);
    } else {
      elsRef.current.delete(id);
      orderRef.current = orderRef.current.filter((x) => x !== id);
    }
  };

  const focusSibling = (id: string, direction: 1 | -1) => {
    const order = orderRef.current;
    const idx = order.indexOf(id);
    if (idx === -1) return;
    const next = order[(idx + direction + order.length) % order.length];
    elsRef.current.get(next)?.focus();
    onChange(next);
  };

  const cls = [
    styles.tablist,
    stretch && styles.stretch,
    accent === 'secondary' && styles.accentSecondary,
    className,
  ]
    .filter(Boolean)
    .join(' ');

  return (
    <TablistContext.Provider value={{ value, onChange, register, focusSibling, accent }}>
      <div role="tablist" aria-label={ariaLabel} className={cls}>
        {children}
      </div>
    </TablistContext.Provider>
  );
}

interface TabProps {
  id: string;
  children: ReactNode;
  className?: string;
  title?: string;
}

export const Tab = forwardRef<HTMLButtonElement, TabProps>(function Tab(
  { id, children, className, title, ...rest },
  forwardedRef,
) {
  const ctx = useContext(TablistContext);
  if (!ctx) throw new Error('Tab must be rendered inside Tablist');

  const active = ctx.value === id;

  const handleKeyDown = (e: KeyboardEvent<HTMLButtonElement>) => {
    if (e.key === 'ArrowRight') {
      e.preventDefault();
      ctx.focusSibling(id, 1);
    } else if (e.key === 'ArrowLeft') {
      e.preventDefault();
      ctx.focusSibling(id, -1);
    }
  };

  const cls = [styles.tab, active && styles.tabActive, className].filter(Boolean).join(' ');

  return (
    <button
      {...rest}
      ref={(el) => {
        ctx.register(id, el);
        if (typeof forwardedRef === 'function') forwardedRef(el);
        else if (forwardedRef) forwardedRef.current = el;
      }}
      role="tab"
      type="button"
      aria-selected={active}
      tabIndex={active ? 0 : -1}
      title={title}
      onClick={() => ctx.onChange(id)}
      onKeyDown={handleKeyDown}
      className={cls}
    >
      {children}
    </button>
  );
});
