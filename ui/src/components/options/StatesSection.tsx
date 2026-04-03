import { PlusIcon } from '@radix-ui/react-icons';
import { StateSquare } from './StateSquare';
import {
  requestAddBlockState,
  requestRecallBlockState,
  requestDeleteBlockState,
} from '../../bridge';
import styles from './StatesSection.module.css';
import type { GridBlock } from '../../store';

interface Props {
  block: GridBlock;
}

export function StatesSection({ block }: Props) {
  return (
    <>
      <div className={styles.divider} />
      <div className={styles.container}>
        <div className={styles.sectionTitle}>States</div>

        <div className={styles.grid}>
          {Array.from({ length: block.numStates ?? 1 }, (_, i) => {
            const isActive = i === (block.activeStateIndex ?? 0);
            const isDirty = (block.dirtyStates ?? []).includes(i);
            return (
              <StateSquare
                key={i}
                index={i}
                isActive={isActive}
                isDirty={isDirty}
                canDelete={(block.numStates ?? 1) > 1}
                onRecall={() => requestRecallBlockState(block.id, i)}
                onDelete={() => requestDeleteBlockState(block.id, i)}
              />
            );
          })}

          {(block.numStates ?? 1) < 16 && (
            <button
              onClick={() => requestAddBlockState(block.id)}
              title="Add new state"
              className={styles.addButton}
            >
              <PlusIcon width={14} height={14} />
            </button>
          )}
        </div>
      </div>
    </>
  );
}
