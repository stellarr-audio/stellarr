import { useState } from 'react';
import { DropdownMenu } from 'radix-ui';
import { IconButton } from '../common/IconButton';
import { Tooltip } from '../common/Tooltip';
import { TbCheck, TbChevronRight, TbDots, TbSquare, TbTrash } from 'react-icons/tb';
import { blockPalette } from '../common/colors';
import { requestSetBlockColor, requestRemoveBlock } from '../../bridge';
import type { GridBlock } from '../../store';
import styles from './OptionsPanel.module.css';

interface Props {
  block: GridBlock;
}

export function OptionsMenu({ block }: Props) {
  const [idCopied, setIdCopied] = useState(false);
  const currentColor = block.blockColor || blockPalette[0];

  const copyId = () => {
    navigator.clipboard.writeText(block.id);
    setIdCopied(true);
    window.setTimeout(() => setIdCopied(false), 1200);
  };

  return (
    <DropdownMenu.Root>
      <Tooltip content="More options">
        <DropdownMenu.Trigger asChild>
          <IconButton
            icon={<TbDots />}
            size="sm"
            title="More options"
          />
        </DropdownMenu.Trigger>
      </Tooltip>
      <DropdownMenu.Portal>
        <DropdownMenu.Content
          sideOffset={6}
          align="end"
          className={styles.menuContent}
          onCloseAutoFocus={(e) => e.preventDefault()}
        >
          <DropdownMenu.Item
            onSelect={(e) => {
              e.preventDefault();
              copyId();
            }}
            className={`${styles.menuItem} ${idCopied ? styles.menuItemCopied : ''}`}
          >
            {idCopied ? <TbCheck size={14} /> : <TbSquare size={14} />}
            <span>{idCopied ? 'Copied' : 'Copy ID'}</span>
          </DropdownMenu.Item>

          <DropdownMenu.Sub>
            <DropdownMenu.SubTrigger className={styles.menuItem}>
              <span
                aria-hidden
                style={{
                  width: 14,
                  height: 14,
                  background: currentColor,
                  border: '1px solid var(--color-border)',
                }}
              />
              <span style={{ flex: 1 }}>Colour</span>
              <TbChevronRight size={12} />
            </DropdownMenu.SubTrigger>
            <DropdownMenu.Portal>
              <DropdownMenu.SubContent sideOffset={4} className={styles.menuSubContent}>
                {blockPalette.map((c) => (
                  <DropdownMenu.Item
                    key={c}
                    asChild
                    onSelect={() => requestSetBlockColor(block.id, c)}
                  >
                    <button
                      type="button"
                      className={`${styles.swatch} ${c === currentColor ? styles.swatchSelected : ''}`}
                      style={{ background: c }}
                      title={c}
                    />
                  </DropdownMenu.Item>
                ))}
              </DropdownMenu.SubContent>
            </DropdownMenu.Portal>
          </DropdownMenu.Sub>

          <DropdownMenu.Separator className={styles.menuSeparator} />

          <DropdownMenu.Item
            onSelect={() => requestRemoveBlock(block.id)}
            className={`${styles.menuItem} ${styles.menuItemDanger}`}
          >
            <TbTrash size={14} />
            <span>Delete block</span>
          </DropdownMenu.Item>
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}
