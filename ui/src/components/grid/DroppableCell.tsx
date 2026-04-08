import { memo } from 'react';
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

export const DroppableCell = memo(function DroppableCell({
  col,
  row,
  occupied,
  isHovered,
  isMenuOpen,
  onMouseEnter,
  onMouseLeave,
  onClick,
  children,
}: Props) {
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

  return (
    <div
      ref={setNodeRef}
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
});
