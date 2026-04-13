import { useEffect, useRef } from 'react';
import { useStore } from '../../store';
import { requestSetLufsWindow } from '../../bridge';
import styles from './LoudnessWindowPopup.module.css';

interface Props {
  open: boolean;
  onClose: () => void;
  anchorRect: DOMRect | null;
}

export function LoudnessWindowPopup({ open, onClose, anchorRect }: Props) {
  const lufsWindow = useStore((s) => s.lufsWindow);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (!open) return;
    const handler = (e: MouseEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node)) onClose();
    };
    window.addEventListener('mousedown', handler);
    return () => window.removeEventListener('mousedown', handler);
  }, [open, onClose]);

  if (!open || !anchorRect) return null;

  const select = (window: 'momentary' | 'shortTerm') => {
    requestSetLufsWindow(window);
    onClose();
  };

  return (
    <div
      ref={ref}
      className={styles.popup}
      style={{ top: anchorRect.bottom + 4, left: anchorRect.left }}
    >
      <button
        className={`${styles.option} ${lufsWindow === 'momentary' ? styles.active : ''}`}
        onClick={() => select('momentary')}
      >
        Momentary (400 ms)
      </button>
      <button
        className={`${styles.option} ${lufsWindow === 'shortTerm' ? styles.active : ''}`}
        onClick={() => select('shortTerm')}
      >
        Short-term (3 s)
      </button>
    </div>
  );
}
