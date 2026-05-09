import { useCallback, useEffect, useRef } from 'react';
import { useDrag } from '@use-gesture/react';
import { IoCloseSharp } from 'react-icons/io5';
import { useStore } from '../../store';
import { IconButton } from '../common/IconButton';
import { MidiMonitorContent } from './MidiMonitor';
import styles from './FloatingMidiPanel.module.css';

const PANEL_EDGE_GUTTER = 16;

export function FloatingMidiPanel() {
  const open = useStore((s) => s.midiPanelOpen);
  const setOpen = useStore((s) => s.setMidiPanelOpen);
  const storedPos = useStore((s) => s.midiPanelPosition);
  const setPos = useStore((s) => s.setMidiPanelPosition);

  const panelRef = useRef<HTMLDivElement | null>(null);

  // Clamp stored position when the parent area resizes. Reserves
  // PANEL_EDGE_GUTTER on every side so the panel never kisses the edge.
  useEffect(() => {
    if (!storedPos) return undefined;
    const onResize = () => {
      const parent = panelRef.current?.parentElement;
      const panelEl = panelRef.current;
      if (!parent || !panelEl) return;
      const maxX = parent.clientWidth - panelEl.offsetWidth - PANEL_EDGE_GUTTER;
      const maxY = parent.clientHeight - panelEl.offsetHeight - PANEL_EDGE_GUTTER;
      const nx = Math.max(PANEL_EDGE_GUTTER, Math.min(storedPos.x, Math.max(PANEL_EDGE_GUTTER, maxX)));
      const ny = Math.max(PANEL_EDGE_GUTTER, Math.min(storedPos.y, Math.max(PANEL_EDGE_GUTTER, maxY)));
      if (nx !== storedPos.x || ny !== storedPos.y) setPos({ x: nx, y: ny });
    };
    window.addEventListener('resize', onResize);
    return () => window.removeEventListener('resize', onResize);
  }, [storedPos, setPos]);

  const getBounds = useCallback(() => {
    const parent = panelRef.current?.parentElement;
    const panelEl = panelRef.current;
    if (!parent || !panelEl) return { left: 0, top: 0, right: 0, bottom: 0 };
    return {
      left: PANEL_EDGE_GUTTER,
      top: PANEL_EDGE_GUTTER,
      right: Math.max(
        PANEL_EDGE_GUTTER,
        parent.clientWidth - panelEl.offsetWidth - PANEL_EDGE_GUTTER,
      ),
      bottom: Math.max(
        PANEL_EDGE_GUTTER,
        parent.clientHeight - panelEl.offsetHeight - PANEL_EDGE_GUTTER,
      ),
    };
  }, []);

  const bindDrag = useDrag(
    ({ offset: [x, y] }) => {
      setPos({ x, y });
    },
    {
      from: () => {
        const pos = useStore.getState().midiPanelPosition;
        if (pos) return [pos.x, pos.y];
        // Default top-left so it doesn't collide with the right-anchored
        // OptionsPanel default. Stored position wins once the user drags.
        return [PANEL_EDGE_GUTTER, PANEL_EDGE_GUTTER];
      },
      bounds: getBounds,
      filterTaps: true,
      pointer: { capture: false },
    },
  );

  if (!open) return null;

  const inlineStyle: React.CSSProperties = storedPos
    ? { left: `${storedPos.x}px`, top: `${storedPos.y}px` }
    : { left: `${PANEL_EDGE_GUTTER}px`, top: `${PANEL_EDGE_GUTTER}px` };

  return (
    <div ref={panelRef} data-floating-panel className={styles.panel} style={inlineStyle}>
      <div className={styles.titlebar}>
        <div {...bindDrag()} className={styles.dragHandle}>
          <span className={styles.titlebarText}>MIDI test</span>
        </div>
        <IconButton
          icon={<IoCloseSharp size={14} />}
          size="sm"
          title="Close MIDI test panel"
          onClick={() => setOpen(false)}
          className={styles.closeBtn}
        />
      </div>
      <div className={styles.content}>
        <MidiMonitorContent boundedLog />
      </div>
    </div>
  );
}
