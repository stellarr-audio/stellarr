import type { ElementType, ComponentPropsWithoutRef, ReactNode } from 'react';
import styles from './Numeric.module.css';

type NumericProps<T extends ElementType> = {
  /** Element to render. Defaults to `span`. */
  as?: T;
  className?: string;
  children: ReactNode;
} & Omit<ComponentPropsWithoutRef<T>, 'as' | 'className' | 'children'>;

/**
 * Wraps numeric / machine-identifier text (dB, Hz, cents, MIDI labels,
 * sample/buffer counts, version strings, format tags) so it renders in
 * JetBrains Mono with tabular, slashed-zero figures. Layout-only `className`;
 * never restyle colours/sizing here — that is the consuming context's job.
 */
export function Numeric<T extends ElementType = 'span'>({
  as,
  className,
  children,
  ...rest
}: NumericProps<T>) {
  const Component = (as ?? 'span') as ElementType;
  return (
    // Cast rest to silence the strict-mode union-spread error on a polymorphic
    // component. The public API (as, className, children, forwarded props) is
    // fully type-safe at the call site via NumericProps<T>.
    <Component
      className={[styles.numeric, className].filter(Boolean).join(' ')}
      {...(rest as Record<string, unknown>)}
    >
      {children}
    </Component>
  );
}
