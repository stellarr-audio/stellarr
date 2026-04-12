import { useCallback, useState } from 'react';
import { useDraggable } from '@dnd-kit/core';
import {
  CopyIcon,
  Cross2Icon,
  ExclamationTriangleIcon,
  SpeakerLoudIcon,
} from '@radix-ui/react-icons';
import type { GridBlock as GridBlockData } from '../../store';
import { requestCopyBlock, requestRemoveBlock, requestOpenPluginEditor } from '../../bridge';
import { useStore } from '../../store';
import { colors } from '../common/colors';
import { PALETTE } from '../options/ColorPicker';
import { CELL_SIZE, cellLeft, cellTop } from './layout';
import { TYPE_ABBREVIATIONS } from '../common/constants';
import styles from './GridBlock.module.css';

interface Props {
  block: GridBlockData;
  onEdgeContextMenu?: (e: React.MouseEvent, blockId: string, side: 'input' | 'output') => void;
}

const typeColors: Record<string, string> = {
  input: PALETTE.slateLight,
  output: PALETTE.slateLight,
  plugin: PALETTE.blue,
};

function EdgeZone({
  side,
  blockId,
  accentColor,
  onEdgeContextMenu,
}: {
  side: 'input' | 'output';
  blockId: string;
  accentColor: string;
  onEdgeContextMenu?: (e: React.MouseEvent, blockId: string, side: 'input' | 'output') => void;
}) {
  const [hovered, setHovered] = useState(false);
  const setDraggingConnection = useStore((s) => s.setDraggingConnection);
  const draggingConnection = useStore((s) => s.draggingConnection);
  const isLeft = side === 'input';

  // Show highlight when hovered OR when a connection is being dragged toward this side
  const isDragTarget =
    draggingConnection !== null &&
    ((draggingConnection.portType === 'output' && isLeft) ||
      (draggingConnection.portType === 'input' && !isLeft));
  const showHighlight = hovered || isDragTarget;

  const handleMouseDown = useCallback(
    (e: React.MouseEvent) => {
      e.stopPropagation();
      e.preventDefault();
      setDraggingConnection({
        blockId,
        portType: side,
        mouseX: e.clientX,
        mouseY: e.clientY,
      });
    },
    [blockId, side, setDraggingConnection],
  );

  return (
    <>
      {showHighlight && (
        <div
          className={`${styles.edgeHighlight} ${isLeft ? styles.edgeHighlightLeft : styles.edgeHighlightRight}`}
          style={{ background: accentColor }}
        />
      )}
      <div
        data-port-block-id={blockId}
        data-port-type={side}
        className={`${styles.edgeZone} ${isLeft ? styles.edgeZoneLeft : styles.edgeZoneRight}`}
        onPointerDown={(e) => e.stopPropagation()}
        onMouseEnter={() => setHovered(true)}
        onMouseLeave={() => setHovered(false)}
        onMouseDown={handleMouseDown}
        onClick={(e) => {
          e.stopPropagation();
          if (onEdgeContextMenu) onEdgeContextMenu(e, blockId, side);
        }}
      />
    </>
  );
}

export function GridBlockComponent({ block, onEdgeContextMenu }: Props) {
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const selectBlock = useStore((s) => s.selectBlock);
  const { attributes, listeners, setNodeRef, isDragging } = useDraggable({
    id: block.id,
  });

  const isSelected = selectedBlockId === block.id;

  const accentColor = block.pluginMissing
    ? colors.warning
    : block.blockColor || typeColors[block.type] || colors.secondary;

  // Border must stay inline — it depends on multiple dynamic values (isSelected, bypassed, accentColor)
  const borderStyle = isSelected
    ? `2px ${block.bypassed ? 'dashed' : 'solid'} #ffffff`
    : block.bypassed
      ? `2px dashed ${accentColor}cc`
      : `1px solid ${accentColor}cc`;

  return (
    <div
      ref={setNodeRef}
      {...listeners}
      {...attributes}
      onClick={() => selectBlock(block.id)}
      onDoubleClick={() => {
        if (block.type === 'plugin' && block.pluginId && !block.pluginMissing)
          requestOpenPluginEditor(block.id);
      }}
      className={styles.block}
      style={{
        left: cellLeft(block.col),
        top: cellTop(block.row),
        width: CELL_SIZE,
        height: CELL_SIZE,
        border: borderStyle,
        opacity: isDragging ? 0.3 : 1,
      }}
    >
      {/* Top region — status icons / format tag */}
      <div className={styles.topRegion}>
        {block.type === 'input' && block.testTone && (
          <SpeakerLoudIcon width={12} height={12} color={colors.green} />
        )}
        {block.pluginMissing && (
          <ExclamationTriangleIcon width={14} height={14} color={colors.warning} />
        )}
        {block.type === 'plugin' && block.pluginFormat && !block.pluginMissing && (
          <span className={styles.formatTag}>
            {block.pluginFormat === 'AudioUnit' ? 'AU' : block.pluginFormat}
          </span>
        )}
      </div>

      {/* Middle region — block type */}
      <div className={styles.blockType} style={{ color: accentColor }}>
        {block.displayName ||
          TYPE_ABBREVIATIONS[block.type] ||
          block.type.slice(0, 3).toUpperCase()}
      </div>

      {/* Bottom region — subtitle */}
      <div className={styles.bottomRegion}>
        {block.type === 'plugin' && (
          <div className={block.pluginMissing ? styles.pluginNameMissing : styles.pluginName}>
            {block.pluginMissing
              ? `Missing: ${block.pluginName || 'Unknown'}`
              : (block.pluginName ?? 'No plugin')}
          </div>
        )}
      </div>

      {/* Input edge zone (left) */}
      {block.type !== 'input' && (
        <EdgeZone
          side="input"
          blockId={block.id}
          accentColor={accentColor}
          onEdgeContextMenu={onEdgeContextMenu}
        />
      )}

      {/* Output edge zone (right) */}
      {block.type !== 'output' && (
        <EdgeZone
          side="output"
          blockId={block.id}
          accentColor={accentColor}
          onEdgeContextMenu={onEdgeContextMenu}
        />
      )}

      {/* Copy button — visible on block hover via CSS */}
      <div
        className={styles.copyButton}
        onPointerDown={(e) => e.stopPropagation()}
        onClick={(e) => {
          e.stopPropagation();
          selectBlock(block.id);
          requestCopyBlock(block.id);
        }}
      >
        <CopyIcon width={12} height={12} />
      </div>

      {/* Remove button — visible on block hover via CSS */}
      <div
        className={styles.removeButton}
        onPointerDown={(e) => e.stopPropagation()}
        onClick={(e) => {
          e.stopPropagation();
          requestRemoveBlock(block.id);
        }}
      >
        <Cross2Icon width={14} height={14} />
      </div>
    </div>
  );
}
