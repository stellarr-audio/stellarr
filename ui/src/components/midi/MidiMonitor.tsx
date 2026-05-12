import { useState, useEffect, useRef } from 'react';
import { useStore } from '../../store';
import { requestInjectMidiCC, requestInjectMidiPC } from '../../bridge';
import { Button } from '../common/Button';
import { Input } from '../common/Input';
import { InputGroup, InputGroupLabel } from '../common/InputGroup';
import { Numeric } from '../common/Numeric';
import { Tablist, Tab } from '../common/Tablist';
import styles from './MidiMonitor.module.css';

type SenderMode = 'cc' | 'pc';

export function MidiMonitor() {
  return (
    <div className={styles.container}>
      <span className={styles.panelTitle}>MIDI</span>
      <div className={styles.divider} />
      <MidiMonitorContent />
    </div>
  );
}

/** Inner content used by both the MIDI tab side-rail and the floating
 * MIDI test panel. Excludes the chrome wrapper + panel title.
 *
 * `boundedLog` caps the log at a fixed height (rather than the default
 * flex-grow behaviour). Use it when the surrounding container is
 * height-constrained — e.g. the floating panel — so the Send CC controls
 * stay anchored as the log fills. */
export function MidiMonitorContent({ boundedLog = false }: { boundedLog?: boolean } = {}) {
  const events = useStore((s) => s.midiMonitorEvents);
  const clearMonitor = useStore((s) => s.clearMidiMonitor);
  const logRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (logRef.current) logRef.current.scrollTop = logRef.current.scrollHeight;
  }, [events]);

  const logClass = boundedLog ? `${styles.log} ${styles.logBounded}` : styles.log;

  return (
    <>
      <div className={styles.monitorSection}>
        <div className={styles.sectionHeader}>
          <span className={styles.sectionTitle}>Monitor</span>
          <Button size="sm" onClick={clearMonitor}>
            Clear
          </Button>
        </div>

        <div ref={logRef} className={logClass}>
          {events.length === 0 ? (
            <span className={styles.logEmpty}>Waiting for MIDI...</span>
          ) : (
            events.map((e, i) => (
              <div key={i} className={styles.logEntry}>
                <span className={styles.logType}>{e.type}</span>{' '}
                <Numeric className={styles.logChannel}>Ch{e.channel + 1}</Numeric>{' '}
                <Numeric className={styles.logData}>
                  {e.type === 'CC'
                    ? `CC${e.data1}=${e.data2}`
                    : e.type === 'Note On' || e.type === 'Note Off'
                      ? `Note${e.data1} Vel${e.data2}`
                      : e.type === 'PC'
                        ? `#${e.data1}`
                        : `${e.data1}`}
                </Numeric>
              </div>
            ))
          )}
        </div>
      </div>

      <div className={styles.divider} />

      <div className={styles.senderSection}>
        <Sender />
      </div>
    </>
  );
}

function Sender() {
  const [mode, setMode] = useState<SenderMode>('cc');
  return (
    <>
      <span className={styles.sectionTitle}>Send</span>
      <Tablist
        value={mode}
        onChange={(id) => setMode(id as SenderMode)}
        aria-label="MIDI message type"
        stretch
        accent="secondary"
      >
        <Tab id="cc">CC</Tab>
        <Tab id="pc">PC</Tab>
      </Tablist>
      {mode === 'cc' ? <CcSender /> : <PcSender />}
    </>
  );
}

function CcSender() {
  const [channel, setChannel] = useState(0);
  const [cc, setCc] = useState(1);
  const [value, setValue] = useState(64);

  const send = () => requestInjectMidiCC(channel, cc, value);

  return (
    <div className={styles.senderFields}>
      <InputGroup>
        <InputGroupLabel className={styles.prefix}>CC#</InputGroupLabel>
        <Input
          inGroup
          mono
          type="number"
          min={0}
          max={127}
          value={cc}
          onChange={(e) => setCc(Math.max(0, Math.min(127, parseInt(e.target.value) || 0)))}
        />
      </InputGroup>

      <InputGroup>
        <InputGroupLabel className={styles.prefix}>Ch</InputGroupLabel>
        <Input
          inGroup
          mono
          type="number"
          min={1}
          max={16}
          value={channel + 1}
          onChange={(e) =>
            setChannel(Math.max(0, Math.min(15, (parseInt(e.target.value) || 1) - 1)))
          }
        />
      </InputGroup>

      <InputGroup>
        <InputGroupLabel className={styles.prefix}>Val</InputGroupLabel>
        <Input
          inGroup
          mono
          type="number"
          min={0}
          max={127}
          value={value}
          onChange={(e) => setValue(Math.max(0, Math.min(127, parseInt(e.target.value) || 0)))}
        />
      </InputGroup>

      <Button onClick={send} className={styles.sendBtn}>
        Send
      </Button>
    </div>
  );
}

function PcSender() {
  const [channel, setChannel] = useState(0);
  const [program, setProgram] = useState(0);

  const send = () => requestInjectMidiPC(channel, program);

  return (
    <div className={styles.senderFields}>
      <InputGroup>
        <InputGroupLabel className={styles.prefix}>Prog</InputGroupLabel>
        <Input
          inGroup
          mono
          type="number"
          min={0}
          max={127}
          value={program}
          onChange={(e) =>
            setProgram(Math.max(0, Math.min(127, parseInt(e.target.value) || 0)))
          }
        />
      </InputGroup>

      <InputGroup>
        <InputGroupLabel className={styles.prefix}>Ch</InputGroupLabel>
        <Input
          inGroup
          mono
          type="number"
          min={1}
          max={16}
          value={channel + 1}
          onChange={(e) =>
            setChannel(Math.max(0, Math.min(15, (parseInt(e.target.value) || 1) - 1)))
          }
        />
      </InputGroup>

      <Button onClick={send} className={styles.sendBtn}>
        Send
      </Button>
    </div>
  );
}
