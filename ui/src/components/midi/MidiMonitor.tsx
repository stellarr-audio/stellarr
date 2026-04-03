import { useState, useEffect, useRef } from 'react';
import { useStore } from '../../store';
import { Slider } from '../common/Slider';
import { requestSetMidiMonitorEnabled, requestInjectMidiCC } from '../../bridge';
import { colors } from '../common/colors';

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
    <div
      style={{
        width: 280,
        flexShrink: 0,
        boxSizing: 'border-box',
        background: '#0f0d1e',
        borderLeft: `1px solid ${colors.border}`,
        padding: '1rem',
        display: 'flex',
        flexDirection: 'column',
        gap: '1rem',
        overflow: 'hidden',
      }}
    >
      {/* Monitor */}
      <div
        style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', flex: 1, minHeight: 0 }}
      >
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <span
            style={{
              fontSize: '1rem',
              fontWeight: 600,
              color: colors.muted,
              letterSpacing: '0.12em',
              textTransform: 'uppercase',
            }}
          >
            Monitor
          </span>
          <button
            onClick={clearMonitor}
            style={{
              background: 'transparent',
              border: `1px solid ${colors.border}`,
              color: colors.muted,
              padding: '0.15rem 0.4rem',
              fontSize: '0.75rem',
              cursor: 'pointer',
            }}
          >
            Clear
          </button>
        </div>

        <div
          ref={logRef}
          style={{
            flex: 1,
            overflow: 'auto',
            background: colors.bg,
            border: `1px solid ${colors.border}`,
            padding: '0.3rem',
            fontSize: '0.75rem',
            fontFamily: 'monospace',
            color: colors.text,
            minHeight: 120,
          }}
        >
          {events.length === 0 ? (
            <span style={{ color: colors.muted, fontStyle: 'italic' }}>Waiting for MIDI...</span>
          ) : (
            events.map((e, i) => (
              <div
                key={i}
                style={{ padding: '0.1rem 0', borderBottom: `1px solid ${colors.border}22` }}
              >
                <span style={{ color: colors.secondary }}>{e.type}</span>{' '}
                <span style={{ color: colors.muted }}>Ch{e.channel + 1}</span>{' '}
                <span style={{ color: colors.text }}>
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
      <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
        <span
          style={{
            fontSize: '1rem',
            fontWeight: 600,
            color: colors.muted,
            letterSpacing: '0.12em',
            textTransform: 'uppercase',
          }}
        >
          Send CC
        </span>
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
    <div style={{ display: 'flex', flexDirection: 'column', gap: '0.4rem' }}>
      <div style={{ display: 'flex', gap: '0.3rem' }}>
        <div style={{ flex: 1 }}>
          <label style={{ fontSize: '0.7rem', color: colors.muted, textTransform: 'uppercase' }}>
            CC#
          </label>
          <input
            type="number"
            min={0}
            max={127}
            value={cc}
            onChange={(e) => setCc(Math.max(0, Math.min(127, parseInt(e.target.value) || 0)))}
            style={{
              width: '100%',
              background: colors.bg,
              border: `1px solid ${colors.border}`,
              color: colors.text,
              fontSize: '0.85rem',
              padding: '0.25rem 0.3rem',
              outline: 'none',
              boxSizing: 'border-box',
            }}
          />
        </div>
        <div style={{ flex: 1 }}>
          <label style={{ fontSize: '0.7rem', color: colors.muted, textTransform: 'uppercase' }}>
            Ch
          </label>
          <input
            type="number"
            min={1}
            max={16}
            value={channel + 1}
            onChange={(e) =>
              setChannel(Math.max(0, Math.min(15, (parseInt(e.target.value) || 1) - 1)))
            }
            style={{
              width: '100%',
              background: colors.bg,
              border: `1px solid ${colors.border}`,
              color: colors.text,
              fontSize: '0.85rem',
              padding: '0.25rem 0.3rem',
              outline: 'none',
              boxSizing: 'border-box',
            }}
          />
        </div>
      </div>

      <div>
        <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '0.2rem' }}>
          <label style={{ fontSize: '0.7rem', color: colors.muted, textTransform: 'uppercase' }}>
            Value
          </label>
          <span
            style={{
              fontSize: '0.75rem',
              color: colors.secondary,
              fontVariantNumeric: 'tabular-nums',
            }}
          >
            {value}
          </span>
        </div>
        <Slider min={0} max={127} value={value} onChange={setValue} />
      </div>

      <button
        onClick={send}
        style={{
          background: colors.primary,
          border: 'none',
          color: '#ffffff',
          padding: '0.35rem',
          fontSize: '0.85rem',
          fontWeight: 600,
          cursor: 'pointer',
          width: '100%',
        }}
      >
        Send
      </button>
    </div>
  );
}
