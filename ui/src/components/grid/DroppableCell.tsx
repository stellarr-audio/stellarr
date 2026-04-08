import { forwardRef, memo } from 'react';
import { useDroppable } from '@dnd-kit/core';
import { CELL_SIZE, cellLeft, cellTop } from './layout';
import styles from './Grid.module.css';

interface Props {
  col: number;
  row: number;
  occupied: boolean;
  isHovered: boolean;
  isMenuOpen: boolean;
  onMouseEnter: () => void;
  onMouseLeave: () => void;
  onClick: () => void;
  children?: React.ReactNode;
}

export const DroppableCell = memo(
  forwardRef<HTMLDivElement, Props>(function DroppableCell(
    { col, row, occupied, isHovered, isMenuOpen, onMouseEnter, onMouseLeave, onClick, children },
    forwardedRef,
  ) {
    const { isOver, setNodeRef } = useDroppable({
      id: `cell-${col}-${row}`,
      disabled: occupied,
    });

    const cellClassName = [
      styles.cell,
      occupied && styles.cellOccupied,
      isHovered && styles.cellHovered,
      isMenuOpen && styles.cellActive,
      isOver && !occupied && styles.cellDropTarget,
    ]
      .filter(Boolean)
      .join(' ');

    // Merge the droppable ref with any forwarded ref (needed for Radix Trigger asChild)
    const mergedRef = (node: HTMLDivElement | null) => {
      setNodeRef(node);
      if (typeof forwardedRef === 'function') forwardedRef(node);
      else if (forwardedRef) forwardedRef.current = node;
    };

    return (
      <div
        ref={mergedRef}
        onMouseEnter={onMouseEnter}
        onMouseLeave={onMouseLeave}
        onClick={onClick}
        className={cellClassName}
        style={{
          left: cellLeft(col),
          top: cellTop(row),
          width: CELL_SIZE,
          height: CELL_SIZE,
        }}
      >
        {children}
      </div>
    );
  }),
);
