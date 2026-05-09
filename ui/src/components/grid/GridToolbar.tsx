import { TbZoomIn, TbZoomOut } from 'react-icons/tb';
import { useStore } from '../../store';
import { IconButton } from '../common/IconButton';
import styles from './GridToolbar.module.css';

export function GridToolbar() {
  const cellZoom = useStore((s) => s.cellZoom);
  const cycleCellZoom = useStore((s) => s.cycleCellZoom);

  return (
    <div className={styles.toolbar}>
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
