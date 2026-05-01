export type TargetKind = 'continuous' | 'binary' | 'none';

export interface TargetMeta {
  kind: TargetKind;

  // Engine-canonical range
  paramRange?: { min: number; max: number };

  // Number-input UX layer (display units)
  paramInputMin?: number;
  paramInputMax?: number;
  paramInputStep?: number;
  paramSuffix?: string;                           // e.g. "%", "dB"; undefined = no suffix
  paramToDisplay?: (canonical: number) => number; // 0.55 → 55 for Mix
  paramFromDisplay?: (display: number) => number; // 55 → 0.55 for Mix

  // Display formatter (for non-input contexts like MidiPage rows)
  paramLabel?: string;
  paramFormat?: (v: number) => string;
  paramParse?: (s: string) => number | null;

  // Binary
  binaryLabels?: { off: string; on: string };
  binaryHelpTemplate?: (threshold: number) => string;
}

const parseNumber = (s: string): number | null => {
  const cleaned = s.replace(/[^0-9.\-]/g, '');
  const v = parseFloat(cleaned);
  return Number.isFinite(v) ? v : null;
};

export const TARGET_META: Record<string, TargetMeta> = {
  blockMix: {
    kind: 'continuous',
    paramRange: { min: 0, max: 1 },
    paramInputMin: 0,
    paramInputMax: 100,
    paramInputStep: 0.5,
    paramSuffix: '%',
    paramToDisplay: (v) => v * 100,
    paramFromDisplay: (d) => Math.max(0, Math.min(1, d / 100)),
    paramLabel: 'Mix value',
    paramFormat: (v) => `${Math.round(v * 100)}%`,
    paramParse: (s) => {
      const n = parseNumber(s);
      return n == null ? null : Math.max(0, Math.min(1, n / 100));
    },
  },
  blockBalance: {
    kind: 'continuous',
    paramRange: { min: -1, max: 1 },
    paramInputMin: -100,
    paramInputMax: 100,
    paramInputStep: 1,
    paramSuffix: undefined,
    paramToDisplay: (v) => v * 100,
    paramFromDisplay: (d) => Math.max(-1, Math.min(1, d / 100)),
    paramLabel: 'Balance',
    paramFormat: (v) => `${Math.round(v * 100)}`,
    paramParse: (s) => {
      const n = parseNumber(s);
      return n == null ? null : Math.max(-1, Math.min(1, n / 100));
    },
  },
  blockLevel: {
    kind: 'continuous',
    paramRange: { min: -60, max: 12 },
    paramInputMin: -60,
    paramInputMax: 12,
    paramInputStep: 0.5,
    paramSuffix: 'dB',
    paramToDisplay: (v) => v,
    paramFromDisplay: (d) => Math.max(-60, Math.min(12, d)),
    paramLabel: 'Level (dB)',
    paramFormat: (v) => `${v.toFixed(1)} dB`,
    paramParse: (s) => {
      const n = parseNumber(s);
      return n == null ? null : Math.max(-60, Math.min(12, n));
    },
  },
  sceneSwitch:  { kind: 'none' },
  presetChange: { kind: 'none' },
  // Binary entries (blockBypass, tunerToggle, blockState) land in Phase B.
};
