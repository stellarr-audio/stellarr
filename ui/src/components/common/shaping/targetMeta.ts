export type TargetKind = 'continuous' | 'binary' | 'none';

export interface TargetMeta {
  kind: TargetKind;

  // Continuous targets
  paramRange?: { min: number; max: number };
  paramFormat?: (v: number) => string;
  paramParse?: (s: string) => number | null;
  paramLabel?: string;

  // Binary targets (populated in Phase B)
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
    paramFormat: (v) => `${Math.round(v * 100)}%`,
    paramParse: (s) => {
      const n = parseNumber(s);
      return n == null ? null : Math.max(0, Math.min(1, n / 100));
    },
    paramLabel: 'Mix value',
  },
  blockBalance: {
    kind: 'continuous',
    paramRange: { min: -1, max: 1 },
    paramFormat: (v) => `${Math.round(v * 100)}`,
    paramParse: (s) => {
      const n = parseNumber(s);
      return n == null ? null : Math.max(-1, Math.min(1, n / 100));
    },
    paramLabel: 'Balance',
  },
  blockLevel: {
    kind: 'continuous',
    paramRange: { min: -60, max: 12 },
    paramFormat: (v) => `${v.toFixed(1)} dB`,
    paramParse: (s) => {
      const n = parseNumber(s);
      return n == null ? null : Math.max(-60, Math.min(12, n));
    },
    paramLabel: 'Level (dB)',
  },
  sceneSwitch:  { kind: 'none' },
  presetChange: { kind: 'none' },
  // Binary entries (blockBypass, tunerToggle, blockState) land in Phase B.
};
