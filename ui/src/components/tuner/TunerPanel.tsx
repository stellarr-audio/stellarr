import { useState } from 'react';
import { useStore } from '../../store';
import { requestSetReferencePitch } from '../../bridge';
import { Input } from '../common/Input';
import { InputGroup, InputGroupLabel } from '../common/InputGroup';
import { Tablist, Tab } from '../common/Tablist';
import { Tag } from '../common/Tag';
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

        <Tablist
          value={tunerMode}
          onChange={(id) => setTunerMode(id as 'needle' | 'strobe')}
          aria-label="Tuner mode"
          stretch
        >
          <Tab id="needle">Needle</Tab>
          <Tab id="strobe">Strobe</Tab>
        </Tablist>
      </div>

      <div className={styles.divider} />

      <div className={styles.content}>
        <span className={styles.sectionTitle}>Reference Pitch</span>

        <InputGroup>
          <InputGroupLabel>A4</InputGroupLabel>
          <Input
            inGroup
            type="number"
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
            <Tag
              key={hz}
              active={referencePitch === hz}
              onClick={() => requestSetReferencePitch(hz)}
              className={styles.presetTag}
            >
              {hz}
            </Tag>
          ))}
        </div>
      </div>
    </div>
  );
}
