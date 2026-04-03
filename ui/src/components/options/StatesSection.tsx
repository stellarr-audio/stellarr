import { PlusIcon } from '@radix-ui/react-icons';
import { StateSquare } from './StateSquare';
import {
  requestAddBlockState,
  requestRecallBlockState,
  requestDeleteBlockState,
} from '../../bridge';
import { colors } from '../common/colors';
import type { GridBlock } from '../../store';

interface Props {
  block: GridBlock;
}

export function StatesSection({ block }: Props) {
  return (
    <>
      <div style={{ height: 1, background: colors.border }} />
      <div style={{ display: 'flex', flexDirection: 'column', gap: '0.75rem' }}>
        <div
          style={{
            fontSize: '1rem',
            fontWeight: 600,
            color: colors.secondary,
            letterSpacing: '0.08em',
            textTransform: 'uppercase',
          }}
        >
          States
        </div>

        <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.4rem' }}>
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
              style={{
                width: 32,
                height: 32,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                background: 'transparent',
                border: `1px dashed ${colors.border}`,
                color: colors.muted,
                cursor: 'pointer',
                padding: 0,
              }}
            >
              <PlusIcon width={14} height={14} />
            </button>
          )}
        </div>
      </div>
    </>
  );
}
