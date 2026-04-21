import { useState } from 'react';
import { DropdownMenu } from 'radix-ui';
import { IconButton } from '../common/IconButton';
import { Tooltip } from '../common/Tooltip';
import { DotsHorizontalIcon, FrameIcon, CheckIcon, ChevronRightIcon } from '@radix-ui/react-icons';
import { BLOCK_COLORS } from './ColorPicker';
import { requestSetBlockColor } from '../../bridge';
import type { GridBlock } from '../../store';
import styles from './OptionsPanel.module.css';

interface Props {
  block: GridBlock;
}

export function OptionsMenu({ block }: Props) {
  const [idCopied, setIdCopied] = useState(false);
  const currentColor = block.blockColor || BLOCK_COLORS[0];

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
            icon={<DotsHorizontalIcon width={14} height={14} />}
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
            {idCopied ? <CheckIcon width={14} height={14} /> : <FrameIcon width={14} height={14} />}
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
              <ChevronRightIcon width={12} height={12} />
            </DropdownMenu.SubTrigger>
            <DropdownMenu.Portal>
              <DropdownMenu.SubContent sideOffset={4} className={styles.menuSubContent}>
                {BLOCK_COLORS.map((c) => (
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
        </DropdownMenu.Content>
      </DropdownMenu.Portal>
    </DropdownMenu.Root>
  );
}
