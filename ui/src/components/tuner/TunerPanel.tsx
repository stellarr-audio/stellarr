import { useState } from 'react';
import { useStore } from '../../store';
import { requestSetReferencePitch } from '../../bridge';
import { InputGroup, InputGroupLabel } from '../common/InputGroup';
import styles from './TunerPanel.module.css';

const presets = [432, 440, 442, 444];

export function TunerPanel() {
  const referencePitch = useStore((s) => s.referencePitch);
  const tunerMode = useStore((s) => s.tunerMode);
  const setTunerMode = useStore((s) => s.setTunerMode);
  const [inputValue, setInputValue] = useState<string>(String(referencePitch));
  const [focused, setFocused] = useState(false);

  const displayValue = focused ? inputValue : String(referencePitch);

  const applyValue = (val: string) => {
    const hz = parseFloat(val);
    if (!isNaN(hz) && hz >= 420 && hz <= 460) {
      requestSetReferencePitch(hz);
    }
    setInputValue(String(referencePitch));
  };

  return (
    <div className={styles.panel}>
      <span className={styles.title}>Tuner</span>

      <div className={styles.divider} />

      <div className={styles.content}>
        <span className={styles.sectionTitle}>Mode</span>

        <div className={styles.modeButtons}>
          <button
            className={`${styles.modeButton} ${tunerMode === 'needle' ? styles.modeButtonActive : ''}`}
            onClick={() => setTunerMode('needle')}
          >
            Needle
          </button>
          <button
            className={`${styles.modeButton} ${tunerMode === 'strobe' ? styles.modeButtonActive : ''}`}
            onClick={() => setTunerMode('strobe')}
          >
            Strobe
          </button>
        </div>
      </div>

      <div className={styles.divider} />

      <div className={styles.content}>
        <span className={styles.sectionTitle}>Reference Pitch</span>

        <InputGroup>
          <InputGroupLabel>A4</InputGroupLabel>
          <input
            type="number"
            className={styles.pitchInput}
            value={displayValue}
            min={420}
            max={460}
            step={1}
            onFocus={() => {
              setInputValue(String(referencePitch));
              setFocused(true);
            }}
            onBlur={(e) => {
              setFocused(false);
              applyValue(e.target.value);
            }}
            onChange={(e) => setInputValue(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter') {
                applyValue(inputValue);
                (e.target as HTMLInputElement).blur();
              }
            }}
          />
          <InputGroupLabel>Hz</InputGroupLabel>
        </InputGroup>

        <div className={styles.presets}>
          {presets.map((hz) => (
            <button
              key={hz}
              className={`${styles.presetButton} ${referencePitch === hz ? styles.presetButtonActive : ''}`}
              onClick={() => requestSetReferencePitch(hz)}
            >
              {hz}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
