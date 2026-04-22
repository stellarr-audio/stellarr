import { IoCloseSharp } from 'react-icons/io5';
import styles from './StateSquare.module.css';

interface Props {
  index: number;
  isActive: boolean;
  isDirty: boolean;
  canDelete: boolean;
  onRecall: () => void;
  onDelete: () => void;
}

export function StateSquare({ index, isActive, isDirty, canDelete, onRecall, onDelete }: Props) {
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
