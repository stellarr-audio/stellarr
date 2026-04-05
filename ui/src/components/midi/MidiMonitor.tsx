import { useState, useEffect, useRef } from 'react';
import { useStore } from '../../store';
import { requestSetMidiMonitorEnabled, requestInjectMidiCC } from '../../bridge';
import styles from './MidiMonitor.module.css';

export function MidiMonitor() {
  const events = useStore((s) => s.midiMonitorEvents);
  const clearMonitor = useStore((s) => s.clearMidiMonitor);
  const setMonitorEnabled = useStore((s) => s.setMidiMonitorEnabled);
  const logRef = useRef<HTMLDivElement>(null);

  // Auto-enable monitor when component mounts
  useEffect(() => {
    requestSetMidiMonitorEnabled(true);
    setMonitorEnabled(true);
    return () => {
      requestSetMidiMonitorEnabled(false);
      setMonitorEnabled(false);
    };
  }, [setMonitorEnabled]);

  // Auto-scroll log
  useEffect(() => {
    if (logRef.current) logRef.current.scrollTop = logRef.current.scrollHeight;
  }, [events]);

  return (
    <div className={styles.container}>
      {/* Monitor */}
      <div className={styles.monitorSection}>
        <div className={styles.sectionHeader}>
          <span className={styles.sectionTitle}>Monitor</span>
          <button onClick={clearMonitor} className={styles.clearBtn}>
            Clear
          </button>
        </div>

        <div ref={logRef} className={styles.log}>
          {events.length === 0 ? (
            <span className={styles.logEmpty}>Waiting for MIDI...</span>
          ) : (
            events.map((e, i) => (
              <div key={i} className={styles.logEntry}>
                <span className={styles.logType}>{e.type}</span>{' '}
                <span className={styles.logChannel}>Ch{e.channel + 1}</span>{' '}
                <span className={styles.logData}>
                  {e.type === 'CC'
                    ? `CC${e.data1}=${e.data2}`
                    : e.type === 'Note On' || e.type === 'Note Off'
                      ? `Note${e.data1} Vel${e.data2}`
                      : e.type === 'PC'
                        ? `#${e.data1}`
                        : `${e.data1}`}
                </span>
              </div>
            ))
          )}
        </div>
      </div>

      {/* Sender */}
      <div className={styles.senderSection}>
        <span className={styles.sectionTitle}>Send CC</span>
        <CcSender />
      </div>
    </div>
  );
}

function CcSender() {
  const [channel, setChannel] = useState(0);
  const [cc, setCc] = useState(1);
  const [value, setValue] = useState(64);

  const send = () => requestInjectMidiCC(channel, cc, value);

  return (
    <div className={styles.senderFields}>
      <div className={styles.fieldRow}>
        <div className={styles.fieldCol}>
          <label className={styles.fieldLabel}>CC#</label>
          <input
            type="number"
            min={0}
            max={127}
            value={cc}
            onChange={(e) => setCc(Math.max(0, Math.min(127, parseInt(e.target.value) || 0)))}
            className={styles.fieldInput}
          />
        </div>
        <div className={styles.fieldCol}>
          <label className={styles.fieldLabel}>Ch</label>
          <input
            type="number"
            min={1}
            max={16}
            value={channel + 1}
            onChange={(e) =>
              setChannel(Math.max(0, Math.min(15, (parseInt(e.target.value) || 1) - 1)))
            }
            className={styles.fieldInput}
          />
        </div>
        <div className={styles.fieldCol}>
          <label className={styles.fieldLabel}>Value</label>
          <input
            type="number"
            min={0}
            max={127}
            value={value}
            onChange={(e) => setValue(Math.max(0, Math.min(127, parseInt(e.target.value) || 0)))}
            className={styles.fieldInput}
          />
        </div>
      </div>

      <button onClick={send} className={styles.sendBtn}>
        Send
      </button>
    </div>
  );
}
