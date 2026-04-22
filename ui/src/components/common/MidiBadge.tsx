import type { MidiMapping } from '../../store';
import { formatMidiLabel } from './constants';
import styles from './MidiBadge.module.css';

interface Props {
  mapping: MidiMapping | null | undefined;
  onClick: () => void;
  title?: string;
  className?: string;
  /** Larger variant — 32h instead of the default 24h. Rarely needed; default
   *  matches IconButton size="sm" so all panel controls share one scale. */
  size?: 'default' | 'lg';
}

/**
 * Compact MIDI-assignment badge. Dashed outline + muted when unassigned,
 * solid orchid when assigned. Shared across the options panel so every
 * MIDI-assignable control reads the same way at a glance.
 */
export function MidiBadge({ mapping, onClick, title, className, size = 'default' }: Props) {
  const label = mapping ? formatMidiLabel(mapping) : 'MIDI';
  const tooltip = title ?? (mapping ? `MIDI: CC ${mapping.cc}` : 'Assign MIDI CC');
  const cls = [
    styles.badge,
    size === 'lg' && styles.lg,
    mapping ? styles.assigned : styles.unassigned,
    className,
  ]
    .filter(Boolean)
    .join(' ');
  return (
    <button type="button" className={cls} onClick={onClick} title={tooltip} aria-label={tooltip}>
      {label}
    </button>
  );
}
