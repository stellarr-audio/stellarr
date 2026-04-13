import { useState, useRef } from 'react';
import { useStore } from '../../store';
import { LoudnessWindowPopup } from './LoudnessWindowPopup';
import styles from './MetricsSection.module.css';

interface Props {
  blockId: string;
}

export function MetricsSection({ blockId }: Props) {
  const lufs = useStore((s) => s.lufsByBlockId[blockId] ?? -60);
  const lufsWindow = useStore((s) => s.lufsWindow);

  const meterRef = useRef<HTMLDivElement>(null);
  const [popupOpen, setPopupOpen] = useState(false);
  const [anchorRect, setAnchorRect] = useState<DOMRect | null>(null);

  const openPopup = () => {
    if (meterRef.current) setAnchorRect(meterRef.current.getBoundingClientRect());
    setPopupOpen(true);
  };

  const widthPct = Math.max(0, Math.min(100, ((lufs + 60) / 60) * 100));
  const tooltip = `Loudness (LUFS, ${lufsWindow === 'momentary' ? 'Momentary 400 ms' : 'Short-term 3 s'}) — perceived volume`;
  const display = lufs <= -59.9 ? '−∞' : lufs.toFixed(1);

  return (
    <div className={styles.section}>
      <div className={styles.label}>
        <span>Loudness</span>
        <span className={styles.value}>{display} LU</span>
      </div>
      <div ref={meterRef} className={styles.meter} title={tooltip} onClick={openPopup}>
        <div className={styles.meterFill} style={{ width: `${widthPct}%` }} />
      </div>
      <LoudnessWindowPopup
        open={popupOpen}
        onClose={() => setPopupOpen(false)}
        anchorRect={anchorRect}
      />
    </div>
  );
}
