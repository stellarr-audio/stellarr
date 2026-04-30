import { IoCloseSharp } from 'react-icons/io5';
import { TbLink } from 'react-icons/tb';
import styles from './StateSquare.module.css';
import type { MidiMapping } from '../../store';
import { PROGRAM_CHANGE_CC } from '../common/constants';

interface Props {
  index: number;
  isActive: boolean;
  isDirty: boolean;
  canDelete: boolean;
  midiMapping?: MidiMapping;
  onRecall: () => void;
  onDelete: () => void;
  onAssignMidi: () => void;
}

export function StateSquare({
  index,
  isActive,
  isDirty,
  canDelete,
  midiMapping,
  onRecall,
  onDelete,
  onAssignMidi,
}: Props) {
  const midiLabel = midiMapping
    ? midiMapping.cc === PROGRAM_CHANGE_CC
      ? `PC ${midiMapping.channel >= 0 ? midiMapping.channel + 1 : '*'}`
      : `CC ${midiMapping.cc}`
    : null;

  return (
    <div className={`${styles.wrapper} ${isActive ? styles.wrapperActive : ''}`}>
      <button
        onClick={() => {
          if (!isActive) onRecall();
        }}
        title={isActive ? `State ${index + 1} (active)` : `Recall state ${index + 1}`}
        className={`${styles.button} ${isDirty ? styles.buttonDirty : isActive ? styles.buttonActive : ''}`}
      >
        {index + 1}
      </button>
      {midiLabel ? (
        <button
          onClick={onAssignMidi}
          title={`MIDI: ${midiLabel}${midiMapping && midiMapping.channel >= 0 ? `, Ch ${midiMapping.channel + 1}` : ''} — click to edit`}
          aria-label={`Edit MIDI assignment for state ${index + 1} (${midiLabel})`}
          className={`${styles.midiSegment} ${styles.midiSegmentAssigned}`}
        >
          {midiLabel}
        </button>
      ) : (
        <button
          onClick={onAssignMidi}
          title={`Assign MIDI to state ${index + 1}`}
          className={`${styles.midiSegment} ${styles.midiIconButton}`}
          aria-label={`Assign MIDI to state ${index + 1}`}
        >
          <TbLink size={12} />
        </button>
      )}
      {canDelete && (
        <button
          onClick={onDelete}
          title={`Delete State ${index + 1}`}
          className={styles.deleteButton}
        >
          <IoCloseSharp size={10} />
        </button>
      )}
    </div>
  );
}
