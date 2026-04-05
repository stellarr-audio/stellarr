import { useEffect, useState, useMemo } from 'react';
import { useStore } from '../../store';
import { Cross2Icon } from '@radix-ui/react-icons';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import { PROGRAM_CHANGE_CC } from '../common/constants';
import {
  requestRemoveMidiMapping,
  requestClearMidiMappings,
  requestGetMidiMappings,
} from '../../bridge';
import styles from './MidiPage.module.css';

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
    <div className={styles.container}>
      <div className={styles.inner}>
        <div className={styles.titleRow}>
          <div className={styles.title}>MIDI Mappings</div>
          {mappings.length > 0 && (
            <button onClick={requestClearMidiMappings} className={styles.clearBtn}>
              Clear All
            </button>
          )}
        </div>

        {mappings.length === 0 ? (
          <div className={styles.emptyState}>
            No MIDI mappings configured.
            <br />
            Use the MIDI buttons next to block parameters to assign CC controls.
          </div>
        ) : (
          <div className={styles.table}>
            {/* Header */}
            <div className={styles.tableHeader}>
              <span className={styles.colActivity}>In</span>
              <span className={styles.colCc}>CC</span>
              <span className={styles.colCh}>Ch</span>
              <span className={styles.colArrowHidden}>&rarr;</span>
              <span className={styles.colTarget}>Target</span>
              <span className={styles.colRemove} />
            </div>

            {/* Rows */}
            {sortedIndices.map((i) => {
              const m = mappings[i];
              const lastActive = activity[i] ?? 0;
              const isActive = Date.now() - lastActive < 500;

              return (
                <div key={i} onClick={() => setEditIndex(i)} className={styles.row}>
                  <span className={styles.activityCol}>
                    <ActivityDot active={isActive} />
                  </span>
                  <span className={styles.ccValue}>{m.cc >= 0 ? m.cc : 'PC'}</span>
                  <span className={styles.chValue}>{m.channel >= 0 ? m.channel + 1 : 'Any'}</span>
                  <span className={styles.arrow}>&rarr;</span>
                  <span className={styles.target}>
                    {targetLabels[m.target] || m.target}
                    {m.blockId ? ` (${blockName(m.blockId)})` : ''}
                  </span>
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      requestRemoveMidiMapping(i);
                    }}
                    className={styles.removeBtn}
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
          programChange={editMapping.cc === PROGRAM_CHANGE_CC}
        />
      )}
    </div>
  );
}

function ActivityDot({ active }: { active: boolean }) {
  return <div className={`${styles.activityDot} ${active ? styles.activityDotActive : ''}`} />;
}
