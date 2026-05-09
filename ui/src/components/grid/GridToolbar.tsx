import { TbZoomIn, TbZoomOut } from 'react-icons/tb';
import { RiRemoteControlLine } from 'react-icons/ri';
import { useStore } from '../../store';
import { IconButton } from '../common/IconButton';
import styles from './GridToolbar.module.css';

export function GridToolbar() {
  const developerModeEnabled = useStore((s) => s.developerModeEnabled);
  const cellZoom = useStore((s) => s.cellZoom);
  const cycleCellZoom = useStore((s) => s.cycleCellZoom);
  const midiPanelOpen = useStore((s) => s.midiPanelOpen);
  const toggleMidiPanel = useStore((s) => s.toggleMidiPanel);

  return (
    <div className={styles.toolbar}>
      {developerModeEnabled && (
        <>
          <IconButton
            icon={<RiRemoteControlLine size={16} />}
            title={midiPanelOpen ? 'Close MIDI test panel' : 'Open MIDI test panel'}
            onClick={toggleMidiPanel}
            active={midiPanelOpen}
          />
          <span className={styles.divider} />
        </>
      )}
      <IconButton
        icon={<TbZoomOut size={16} />}
        title="Zoom out (smaller cells)"
        onClick={() => cycleCellZoom(-1)}
        disabled={cellZoom === 'S'}
      />
      <IconButton
        icon={<TbZoomIn size={16} />}
        title="Zoom in (larger cells)"
        onClick={() => cycleCellZoom(+1)}
        disabled={cellZoom === 'L'}
      />
    </div>
  );
}
