import { useEffect } from 'react';
import { useStore } from '../../store';
import { Cross2Icon } from '@radix-ui/react-icons';
import {
  requestRemoveMidiMapping,
  requestClearMidiMappings,
  requestGetMidiMappings,
} from '../../bridge';
import { colors } from '../common/colors';

const targetLabels: Record<string, string> = {
  presetChange: 'Preset Change',
  sceneSwitch: 'Scene Switch',
  blockBypass: 'Block Bypass',
  blockMix: 'Block Mix',
  blockBalance: 'Block Balance',
  blockLevel: 'Block Level',
  tunerToggle: 'Tuner Toggle',
};

export function MidiSettings() {
  const mappings = useStore((s) => s.midiMappings);
  const blocks = useStore((s) => s.blocks);

  useEffect(() => {
    requestGetMidiMappings();
  }, []);

  const blockName = (id?: string) => {
    if (!id) return '';
    const block = blocks.find((b) => b.id === id);
    return block ? block.displayName || block.name : id.slice(0, 8);
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem' }}>
      {mappings.length === 0 ? (
        <div style={{ color: colors.muted, fontStyle: 'italic', fontSize: '0.9rem' }}>
          No MIDI mappings. Use the MIDI buttons next to block parameters to assign CC controls.
        </div>
      ) : (
        <>
          <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem' }}>
            {mappings.map((m, i) => (
              <div
                key={i}
                style={{
                  display: 'flex',
                  alignItems: 'center',
                  padding: '0.35rem 0.5rem',
                  background: colors.cell,
                  border: `1px solid ${colors.border}`,
                  fontSize: '0.9rem',
                }}
              >
                <span
                  style={{
                    color: colors.text,
                    fontWeight: 600,
                    fontVariantNumeric: 'tabular-nums',
                    minWidth: '6ch',
                  }}
                >
                  CC {m.cc}
                </span>
                <span style={{ color: colors.muted, minWidth: '5ch' }}>
                  {m.channel >= 0 ? `Ch ${m.channel + 1}` : 'Any'}
                </span>
                <span style={{ color: colors.muted, margin: '0 0.4rem' }}>→</span>
                <span style={{ color: colors.secondary, flex: 1 }}>
                  {targetLabels[m.target] || m.target}
                  {m.blockId ? ` (${blockName(m.blockId)})` : ''}
                </span>
                <button
                  onClick={() => requestRemoveMidiMapping(i)}
                  style={{
                    background: 'transparent',
                    border: 'none',
                    color: colors.muted,
                    cursor: 'pointer',
                    padding: '0.1rem',
                    display: 'flex',
                    alignItems: 'center',
                  }}
                >
                  <Cross2Icon width={12} height={12} />
                </button>
              </div>
            ))}
          </div>
          <button
            onClick={requestClearMidiMappings}
            style={{
              alignSelf: 'flex-end',
              background: 'transparent',
              border: `1px solid ${colors.border}`,
              color: colors.muted,
              padding: '0.2rem 0.5rem',
              fontSize: '0.85rem',
              cursor: 'pointer',
            }}
          >
            Clear All
          </button>
        </>
      )}
    </div>
  );
}
