import type { MidiMapping } from '../../store';

/** cc value of -1 indicates a Program Change mapping (not CC). */
export const PROGRAM_CHANGE_CC = -1;

export const TYPE_ABBREVIATIONS: Record<string, string> = {
  input: 'INP',
  output: 'OUT',
  plugin: 'PLG',
};

export function formatMidiLabel(mapping: MidiMapping | null): string {
  if (!mapping) return 'MIDI';
  const ch = mapping.channel >= 0 ? `/Ch${mapping.channel + 1}` : '';
  if (mapping.cc === PROGRAM_CHANGE_CC) return `PC${ch}`;
  return `CC${mapping.cc}${ch}`;
}
