import type { MidiMapping } from '../../store';
import { formatMidiLabel } from './constants';
import styles from './MidiBadge.module.css';

interface Props {
  mapping: MidiMapping | null | undefined;
  onClick: () => void;
  title?: string;
  className?: string;
  /** Compact variant — matches IconButton size="sm" for titlebar clusters. */
  size?: 'default' | 'sm';
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
    size !== 'default' && styles[size],
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
