import type { MidiMapping } from '../../store';

export const TYPE_ABBREVIATIONS: Record<string, string> = {
  input: 'INP',
  output: 'OUT',
  plugin: 'PLG',
};

export function formatMidiLabel(mapping: MidiMapping | null): string {
  if (!mapping) return 'MIDI';
  const ch = mapping.channel >= 0 ? `/Ch${mapping.channel + 1}` : '';
  if (mapping.cc === -1) return `PC${ch}`;
  return `CC${mapping.cc}${ch}`;
}
