import { useEffect, useState, useMemo } from 'react';
import { useStore } from '../../store';
import { Cross2Icon } from '@radix-ui/react-icons';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import {
  requestRemoveMidiMapping,
  requestClearMidiMappings,
  requestGetMidiMappings,
} from '../../bridge';
import { colors } from '../common/colors';

const targetLabels: Record<string, string> = {
  presetChange: 'Preset Change (Global)',
  sceneSwitch: 'Scene Switch',
  blockBypass: 'Block Bypass',
  blockMix: 'Block Mix',
  blockBalance: 'Block Balance',
  blockLevel: 'Block Level',
  tunerToggle: 'Tuner Toggle (Global)',
};

export function MidiPage() {
  const mappings = useStore((s) => s.midiMappings);
  const blocks = useStore((s) => s.blocks);
  const activity = useStore((s) => s.midiMappingActivity);
  const [editIndex, setEditIndex] = useState<number | null>(null);

  const [, setTick] = useState(0);

  useEffect(() => {
    requestGetMidiMappings();
  }, []);

  // Re-render periodically to fade activity dots
  useEffect(() => {
    const interval = setInterval(() => setTick((t) => t + 1), 200);
    return () => clearInterval(interval);
  }, []);

  const blockName = (id?: string) => {
    if (!id) return '';
    const block = blocks.find((b) => b.id === id);
    return block ? block.displayName || block.name : id.slice(0, 8);
  };

  const sortedIndices = useMemo(
    () =>
      mappings
        .map((_, i) => i)
        .sort((a, b) => {
          const ma = mappings[a],
            mb = mappings[b];
          if (ma.cc !== mb.cc) return ma.cc - mb.cc;
          return ma.channel - mb.channel;
        }),
    [mappings],
  );

  const editMapping =
    editIndex !== null && editIndex < mappings.length ? mappings[editIndex] : null;

  return (
    <div
      style={{
        flex: 1,
        display: 'flex',
        flexDirection: 'column',
        background: colors.bg,
        padding: '2rem',
        overflow: 'auto',
      }}
    >
      <div
        style={{
          maxWidth: 700,
          width: '100%',
          margin: '0 auto',
          display: 'flex',
          flexDirection: 'column',
          gap: '1.5rem',
        }}
      >
        <div
          style={{
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
          }}
        >
          <div
            style={{
              fontSize: '1.2rem',
              fontWeight: 700,
              color: colors.text,
              letterSpacing: '0.08em',
              textTransform: 'uppercase',
            }}
          >
            MIDI Mappings
          </div>
          {mappings.length > 0 && (
            <button
              onClick={requestClearMidiMappings}
              style={{
                background: 'transparent',
                border: `1px solid ${colors.border}`,
                color: colors.muted,
                padding: '0.3rem 0.6rem',
                fontSize: '0.85rem',
                cursor: 'pointer',
              }}
            >
              Clear All
            </button>
          )}
        </div>

        {mappings.length === 0 ? (
          <div
            style={{
              color: colors.muted,
              fontStyle: 'italic',
              fontSize: '1rem',
              textAlign: 'center',
              padding: '3rem 0',
            }}
          >
            No MIDI mappings configured.
            <br />
            Use the MIDI buttons next to block parameters to assign CC controls.
          </div>
        ) : (
          <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem' }}>
            {/* Header */}
            <div
              style={{
                display: 'flex',
                alignItems: 'center',
                padding: '0.35rem 0.5rem',
                fontSize: '0.8rem',
                color: colors.muted,
                textTransform: 'uppercase',
                letterSpacing: '0.06em',
              }}
            >
              <span style={{ width: 32, textAlign: 'center' }}>In</span>
              <span style={{ minWidth: '6ch' }}>CC</span>
              <span style={{ minWidth: '5ch' }}>Ch</span>
              <span style={{ margin: '0 0.4rem', visibility: 'hidden' }}>→</span>
              <span style={{ flex: 1 }}>Target</span>
              <span style={{ width: 20 }} />
            </div>

            {/* Rows */}
            {sortedIndices.map((i) => {
              const m = mappings[i];
              const lastActive = activity[i] ?? 0;
              const isActive = Date.now() - lastActive < 500;

              return (
                <div
                  key={i}
                  onClick={() => setEditIndex(i)}
                  style={{
                    display: 'flex',
                    alignItems: 'center',
                    padding: '0.45rem 0.5rem',
                    background: colors.cell,
                    border: `1px solid ${colors.border}`,
                    fontSize: '0.95rem',
                    cursor: 'pointer',
                    transition: 'background 0.1s ease',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.background = colors.border;
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.background = colors.cell;
                  }}
                >
                  <span style={{ width: 32, display: 'flex', justifyContent: 'center' }}>
                    <ActivityDot active={isActive} />
                  </span>
                  <span
                    style={{
                      color: colors.text,
                      fontWeight: 600,
                      fontVariantNumeric: 'tabular-nums',
                      minWidth: '6ch',
                    }}
                  >
                    {m.cc >= 0 ? m.cc : 'PC'}
                  </span>
                  <span style={{ color: colors.muted, minWidth: '5ch' }}>
                    {m.channel >= 0 ? m.channel + 1 : 'Any'}
                  </span>
                  <span style={{ color: colors.muted, margin: '0 0.4rem' }}>→</span>
                  <span style={{ color: colors.secondary, flex: 1 }}>
                    {targetLabels[m.target] || m.target}
                    {m.blockId ? ` (${blockName(m.blockId)})` : ''}
                  </span>
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      requestRemoveMidiMapping(i);
                    }}
                    style={{
                      background: 'transparent',
                      border: 'none',
                      color: colors.muted,
                      cursor: 'pointer',
                      padding: '0.2rem',
                      display: 'flex',
                      alignItems: 'center',
                    }}
                  >
                    <Cross2Icon width={14} height={14} />
                  </button>
                </div>
              );
            })}
          </div>
        )}
      </div>

      {/* Edit dialog */}
      {editMapping && (
        <MidiAssignDialog
          open={editIndex !== null}
          onOpenChange={(open) => {
            if (!open) setEditIndex(null);
          }}
          title={`Edit — ${targetLabels[editMapping.target] || editMapping.target}`}
          target={editMapping.target}
          blockId={editMapping.blockId}
          existingIndex={editIndex ?? undefined}
        />
      )}
    </div>
  );
}

function ActivityDot({ active }: { active: boolean }) {
  return (
    <div
      style={{
        width: 10,
        height: 10,
        transform: 'rotate(45deg)',
        background: active ? colors.green : colors.muted,
        opacity: active ? 1 : 0.3,
        transition: 'background 0.1s ease, opacity 0.4s ease',
        boxShadow: active ? `0 0 4px ${colors.green}88` : 'none',
      }}
    />
  );
}
