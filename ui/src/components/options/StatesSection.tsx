import { useState } from 'react';
import { IoAddSharp } from 'react-icons/io5';
import { StateSquare } from './StateSquare';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import {
  requestAddBlockState,
  requestRecallBlockState,
  requestDeleteBlockState,
} from '../../bridge';
import styles from './StatesSection.module.css';
import { useStore, type GridBlock } from '../../store';

interface Props {
  block: GridBlock;
}

export function StatesSection({ block }: Props) {
  const [dialogState, setDialogState] = useState<{ index: number } | null>(null);
  const midiMappings = useStore((s) => s.midiMappings);
  const getStateMidiMapping = useStore((s) => s.getStateMidiMapping);

  const numStates = block.numStates ?? 1;
  const activeStateIndex = block.activeStateIndex ?? 0;

  const dialogMappingIndex = dialogState
    ? midiMappings.findIndex(
        (m) =>
          m.target === 'blockState'
          && m.blockId === block.id
          && m.targetIndex === dialogState.index,
      )
    : -1;

  return (
    <>
      <div className={styles.divider} />
      <div className={styles.container}>
        <div className={styles.sectionTitle}>States</div>

        <div className={styles.grid}>
          {Array.from({ length: numStates }, (_, i) => {
            const isActive = i === activeStateIndex;
            const isDirty = (block.dirtyStates ?? []).includes(i);
            return (
              <StateSquare
                key={i}
                index={i}
                isActive={isActive}
                isDirty={isDirty}
                canDelete={numStates > 1}
                midiMapping={getStateMidiMapping(block.id, i)}
                onRecall={() => requestRecallBlockState(block.id, i)}
                onDelete={() => requestDeleteBlockState(block.id, i)}
                onAssignMidi={() => setDialogState({ index: i })}
              />
            );
          })}

          {numStates < 16 && (
            <button
              onClick={() => requestAddBlockState(block.id)}
              title="Add new state"
              className={styles.addButton}
            >
              <IoAddSharp size={14} />
            </button>
          )}
        </div>
      </div>

      <MidiAssignDialog
        open={dialogState !== null}
        onOpenChange={(open) => {
          if (!open) setDialogState(null);
        }}
        title={dialogState !== null ? `Assign MIDI to state ${dialogState.index + 1}` : ''}
        target="blockState"
        blockId={block.id}
        targetIndex={dialogState?.index}
        existingIndex={dialogMappingIndex >= 0 ? dialogMappingIndex : undefined}
      />
    </>
  );
}
