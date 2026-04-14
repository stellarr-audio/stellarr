import { useState, useRef } from 'react';
import { useStore } from '../../store';
import { LoudnessWindowPopup } from '../options/LoudnessWindowPopup';
import styles from './SystemStats.module.css';

function cpuBarColor(percent: number): string {
  if (percent < 40) return 'var(--color-green)';
  if (percent < 70) return '#ffcc00';
  return '#ff4444';
}

function CpuBar() {
  const cpu = useStore((s) => s.cpuPercent);
  const clamped = Math.min(100, Math.max(0, cpu));
  const color = cpuBarColor(clamped);

  return (
    <div className={styles.statBar}>
      <span className={styles.label}>CPU</span>
      <div className={styles.track}>
        <div className={styles.fill} style={{ width: `${clamped}%`, background: color }} />
      </div>
      <span className={styles.valueCpu} style={{ color }}>
        {clamped.toFixed(0)}%
      </span>
    </div>
  );
}

function OutputLufsBar() {
  const lufsByBlockId = useStore((s) => s.lufsByBlockId);
  const targetLufsByBlockId = useStore((s) => s.targetLufsByBlockId);
  const blocks = useStore((s) => s.blocks);
  const lufsWindow = useStore((s) => s.lufsWindow);

  // Find the Output block ID
  const outputBlock = blocks.find((b) => b.type === 'output');
  const outputLufs = outputBlock ? (lufsByBlockId[outputBlock.id] ?? -60) : -60;
  const outputTarget = outputBlock ? targetLufsByBlockId[outputBlock.id] : null;

  const meterRef = useRef<HTMLDivElement>(null);
  const [popupOpen, setPopupOpen] = useState(false);
  const [anchorRect, setAnchorRect] = useState<DOMRect | null>(null);

  const openPopup = () => {
    if (meterRef.current) setAnchorRect(meterRef.current.getBoundingClientRect());
    setPopupOpen(true);
  };

  // Map LUFS [-60..0] to width [0..100]
  const lufsWidthPct = Math.max(0, Math.min(100, ((outputLufs + 60) / 60) * 100));

  // Determine colour based on target distance
  let lufsFillColor = 'var(--color-secondary)';
  if (outputTarget != null) {
    const delta = Math.abs(outputLufs - outputTarget);
    if (delta <= 1) lufsFillColor = 'var(--color-green)';
    else if (delta <= 3) lufsFillColor = 'var(--color-warning)';
    else lufsFillColor = 'var(--color-danger)';
  }

  const lufsTooltip = `Loudness (LUFS, ${lufsWindow === 'momentary' ? 'Momentary 400 ms' : 'Short-term 3 s'}) — perceived volume, matching how the ear hears`;

  const display = outputLufs <= -60 ? '-inf' : outputLufs.toFixed(1);

  return (
    <>
      <div className={styles.statBar}>
        <span className={styles.label}>OUT</span>
        <div
          ref={meterRef}
          className={styles.track}
          title={lufsTooltip}
          onClick={openPopup}
          style={{ cursor: 'pointer' }}
        >
          <div
            className={styles.fill}
            style={{ width: `${lufsWidthPct}%`, background: lufsFillColor }}
          />
        </div>
        <span className={styles.valueOut} style={{ color: lufsFillColor }}>
          {display}
        </span>
      </div>
      <LoudnessWindowPopup
        open={popupOpen}
        onClose={() => setPopupOpen(false)}
        anchorRect={anchorRect}
      />
    </>
  );
}

export function SystemStats() {
  return (
    <div className={styles.container}>
      <CpuBar />
      <OutputLufsBar />
    </div>
  );
}
