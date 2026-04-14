import { Tooltip as RadixTooltip } from 'radix-ui';
import type { ReactNode } from 'react';
import styles from './Tooltip.module.css';

interface Props {
  content: ReactNode;
  children: ReactNode;
  /** If provided, controls open state externally (for programmatic show/hide). */
  open?: boolean;
  /** Delay before showing on hover, in ms. Defaults to 200. */
  delayDuration?: number;
  side?: 'top' | 'right' | 'bottom' | 'left';
}

export function Tooltip({ content, children, open, delayDuration = 200, side = 'top' }: Props) {
  return (
    <RadixTooltip.Provider delayDuration={delayDuration}>
      <RadixTooltip.Root open={open}>
        <RadixTooltip.Trigger asChild>{children}</RadixTooltip.Trigger>
        <RadixTooltip.Portal>
          <RadixTooltip.Content className={styles.content} side={side} sideOffset={6}>
            {content}
            <RadixTooltip.Arrow className={styles.arrow} />
          </RadixTooltip.Content>
        </RadixTooltip.Portal>
      </RadixTooltip.Root>
    </RadixTooltip.Provider>
  );
}
