import { useState, useEffect, useMemo, useRef, useCallback } from 'react';
import { useDrag } from '@use-gesture/react';
import { useStore } from '../../store';
import { ToggleSwitch } from '../common/ToggleSwitch';
import { Tooltip } from '../common/Tooltip';
import { Input } from '../common/Input';
import { InputGroup } from '../common/InputGroup';
import { IconButton } from '../common/IconButton';
import { MidiAssignDialog } from '../common/MidiAssignDialog';
import { MidiBadge } from '../common/MidiBadge';
import { TYPE_ABBREVIATIONS } from '../common/constants';
import { Pencil1Icon, PlayIcon, StopIcon, ChevronDownIcon, CheckIcon } from '@radix-ui/react-icons';
import { IoCloseSharp } from 'react-icons/io5';
import { PluginSection } from './PluginSection';
import { ParametersSection } from './ParametersSection';
import { StatesSection } from './StatesSection';
import { SignalSection } from './SignalSection';
import { TargetLoudnessControl } from './TargetLoudnessControl';
import { OptionsMenu } from './OptionsMenu';
import { Select } from 'radix-ui';
import {
  requestToggleTestTone,
  requestToggleBlockBypass,
  requestRenameBlock,
  requestGetTestToneSamples,
  requestSetTestToneSample,
} from '../../bridge';
import styles from './OptionsPanel.module.css';

const PANEL_EDGE_GUTTER = 16;

export function OptionsPanel() {
  const selectedBlockId = useStore((s) => s.selectedBlockId);
  const blocks = useStore((s) => s.blocks);
  const availablePlugins = useStore((s) => s.availablePlugins);
  const selectBlock = useStore((s) => s.selectBlock);
  const storedPos = useStore((s) => s.floatingPanelPos);
  const setFloatingPanelPos = useStore((s) => s.setFloatingPanelPos);

  const block = selectedBlockId ? blocks.find((b) => b.id === selectedBlockId) : null;

  const panelRef = useRef<HTMLDivElement | null>(null);

  // Esc-to-close while a block is selected
  useEffect(() => {
    if (!block) return undefined;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        e.preventDefault();
        selectBlock(null);
      }
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [block, selectBlock]);

  // Clamp panel position inside the parent (Grid tab panel) on resize.
  useEffect(() => {
    if (!storedPos) return undefined;
    const onResize = () => {
      const parent = panelRef.current?.parentElement;
      if (!parent) return;
      const panelEl = panelRef.current;
      if (!panelEl) return;
      const maxX = parent.clientWidth - panelEl.offsetWidth;
      const maxY = parent.clientHeight - panelEl.offsetHeight;
      const nx = Math.max(0, Math.min(storedPos.x, Math.max(0, maxX)));
      const ny = Math.max(0, Math.min(storedPos.y, Math.max(0, maxY)));
      if (nx !== storedPos.x || ny !== storedPos.y) {
        setFloatingPanelPos({ x: nx, y: ny });
      }
    };
    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, [storedPos, setFloatingPanelPos]);

  const getBounds = useCallback(() => {
    const parent = panelRef.current?.parentElement;
    const panelEl = panelRef.current;
    if (!parent || !panelEl) return { left: 0, top: 0, right: 0, bottom: 0 };
    return {
      left: 0,
      top: 0,
      right: Math.max(0, parent.clientWidth - panelEl.offsetWidth),
      bottom: Math.max(0, parent.clientHeight - panelEl.offsetHeight),
    };
  }, []);

  const bindDrag = useDrag(
    ({ offset: [x, y] }) => {
      setFloatingPanelPos({ x, y });
    },
    {
      from: () => {
        const pos = useStore.getState().floatingPanelPos;
        if (pos) return [pos.x, pos.y];
        // Default top-right offset
        const parent = panelRef.current?.parentElement;
        const panelEl = panelRef.current;
        if (!parent || !panelEl) return [0, 0];
        return [
          Math.max(0, parent.clientWidth - panelEl.offsetWidth - PANEL_EDGE_GUTTER),
          PANEL_EDGE_GUTTER,
        ];
      },
      bounds: getBounds,
      filterTaps: true,
      // Don't grab pointer capture — otherwise clicks on child buttons
      // (close, ⋯, bypass toggle) route to the titlebar instead of the target.
      pointer: { capture: false },
    },
  );

  if (!block) return null;

  // Resolve position: stored, else default top-right once the panel is mounted.
  // We render the panel with a placeholder offset first; a layout effect then
  // snaps it into the correct default corner if no stored position exists.
  const inlineStyle: React.CSSProperties = storedPos
    ? { left: `${storedPos.x}px`, top: `${storedPos.y}px` }
    : { right: `${PANEL_EDGE_GUTTER}px`, top: `${PANEL_EDGE_GUTTER}px` };

  return (
    <div ref={panelRef} data-options-panel className={styles.panel} style={inlineStyle}>
      <BlockHeader block={block} onClose={() => selectBlock(null)} bindDrag={bindDrag} />

      <div className={styles.content}>
        {/* Signal — Level slider + loudness history. Same for every block. */}
        <SignalSection block={block} />

        {/* Input block — test tone picker */}
        {block.type === 'input' && (
          <>
            <div className={styles.divider} />
            <TestToneSamplePicker blockId={block.id} playing={block.testTone ?? false} />
          </>
        )}

        {/* Plugin block — plugin select */}
        {block.type === 'plugin' && (
          <>
            <div className={styles.divider} />
            <PluginSection block={block} availablePlugins={availablePlugins} />
          </>
        )}

        {/* Plugin block — parameters (Mix, Balance, Bypass mode) */}
        {block.type === 'plugin' && <ParametersSection block={block} />}

        {/* Output block — target loudness */}
        {block.type === 'output' && (
          <>
            <div className={styles.divider} />
            <TargetLoudnessControl blockId={block.id} />
          </>
        )}

        {/* Plugin block — states */}
        {block.type === 'plugin' && <StatesSection block={block} />}
      </div>
    </div>
  );
}

interface BlockHeaderProps {
  block: import('../../store').GridBlock;
  onClose: () => void;
  bindDrag: () => Record<string, unknown>;
}

function BlockHeader({ block, onClose, bindDrag }: BlockHeaderProps) {
  const [editing, setEditing] = useState(false);
  const [editValue, setEditValue] = useState('');

  const abbreviation = TYPE_ABBREVIATIONS[block.type] || block.type.slice(0, 3).toUpperCase();
  const displayName = block.displayName || abbreviation;

  const startEdit = () => {
    setEditValue(displayName);
    setEditing(true);
  };

  const submitEdit = () => {
    setEditing(false);
    const trimmed = editValue.trim();
    if (trimmed && trimmed !== displayName) {
      requestRenameBlock(block.id, trimmed);
    }
  };

  const hasBypass = block.type !== 'input' && block.type !== 'output';

  return (
    <div className={styles.titlebar}>
      <div {...bindDrag()} className={styles.dragHandle}>
        {editing ? (
          <InputGroup size="sm" className={styles.editGroup}>
            <Input
              autoFocus
              inGroup
              maxLength={3}
              value={editValue}
              onChange={(e) => setEditValue(e.target.value)}
              onBlur={submitEdit}
              onKeyDown={(e) => {
                if (e.key === 'Enter') submitEdit();
                if (e.key === 'Escape') {
                  e.stopPropagation();
                  setEditing(false);
                }
              }}
              className={styles.editInput}
            />
            <IconButton
              icon={<CheckIcon width={12} height={12} />}
              inGroup
              onMouseDown={(e) => {
                // Prevent the input's blur from firing before onClick.
                e.preventDefault();
              }}
              onClick={submitEdit}
              title="Confirm rename"
            />
          </InputGroup>
        ) : (
          <>
            <span
              className={`${styles.blockName} ${block.bypassed ? styles.blockNameBypassed : ''}`}
            >
              {displayName}
            </span>
            <Tooltip content="Rename block">
              <IconButton
                icon={<Pencil1Icon width={12} height={12} />}
                size="sm"
                onClick={startEdit}
                title="Rename block"
              />
            </Tooltip>
          </>
        )}
        <span className={styles.titleSpacer} />
      </div>

      <div className={styles.titlebarControls}>
        {hasBypass && <BypassControls block={block} />}
        <OptionsMenu block={block} />
        <Tooltip content="Close panel">
          <IconButton
            icon={<IoCloseSharp size={14} />}
            size="sm"
            onClick={onClose}
            title="Close panel"
          />
        </Tooltip>
      </div>
    </div>
  );
}

function BypassControls({ block }: { block: import('../../store').GridBlock }) {
  const mappings = useStore((s) => s.midiMappings);
  const [dialogOpen, setDialogOpen] = useState(false);

  const existingIndex = mappings.findIndex(
    (m) => m.target === 'blockBypass' && (m.blockId ?? '') === block.id,
  );
  const existing = existingIndex >= 0 ? mappings[existingIndex] : null;

  const tooltipLabel = existing ? `Bypass MIDI: CC ${existing.cc}` : 'Assign MIDI CC to bypass';

  return (
    <>
      <MidiBadge mapping={existing} onClick={() => setDialogOpen(true)} title={tooltipLabel} />
      <ToggleSwitch
        enabled={!block.bypassed}
        sharp
        onToggle={() => {
          useStore.getState().setBlockBypassed(block.id, !block.bypassed);
          requestToggleBlockBypass(block.id);
        }}
        title={block.bypassed ? 'Enable block' : 'Bypass block'}
      />
      <MidiAssignDialog
        open={dialogOpen}
        onOpenChange={setDialogOpen}
        title="MIDI — Bypass"
        target="blockBypass"
        blockId={block.id}
        existingIndex={existingIndex >= 0 ? existingIndex : undefined}
      />
    </>
  );
}

function TestToneSamplePicker({ blockId, playing }: { blockId: string; playing: boolean }) {
  const samples = useStore((s) => s.testToneSamples);
  const sortedSamples = useMemo(() => samples.toSorted((a, b) => a.localeCompare(b)), [samples]);
  const block = useStore((s) => s.blocks.find((b) => b.id === blockId));
  const currentSample = block?.testToneSample || 'Synth (Default)';

  useEffect(() => {
    requestGetTestToneSamples();
  }, []);

  if (samples.length === 0) return null;

  return (
    <div className={styles.samplePicker}>
      <span className={styles.sampleLabel}>Test Tone</span>
      <InputGroup>
        <Select.Root
          value={currentSample}
          onValueChange={(v) => requestSetTestToneSample(blockId, v)}
        >
          <Select.Trigger className={styles.sampleTrigger}>
            <Select.Value />
            <Select.Icon>
              <ChevronDownIcon />
            </Select.Icon>
          </Select.Trigger>
          <Select.Portal>
            <Select.Content position="popper" sideOffset={4} className={styles.sampleContent}>
              <Select.Viewport>
                {sortedSamples.map((s) => (
                  <Select.Item key={s} value={s} className={styles.sampleItem}>
                    <Select.ItemText>{s}</Select.ItemText>
                  </Select.Item>
                ))}
              </Select.Viewport>
            </Select.Content>
          </Select.Portal>
        </Select.Root>
        <IconButton
          icon={playing ? <StopIcon width={14} height={14} /> : <PlayIcon width={14} height={14} />}
          inGroup
          onClick={() => requestToggleTestTone(blockId)}
          title={playing ? 'Stop test tone' : 'Play test tone'}
          className={playing ? styles.toneButtonPlaying : undefined}
        />
      </InputGroup>
    </div>
  );
}
