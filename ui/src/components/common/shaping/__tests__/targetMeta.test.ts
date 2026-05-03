import { describe, it, expect } from 'vitest';
import { TARGET_META } from '../targetMeta';

describe('TARGET_META', () => {
  it('declares blockMix as continuous with paramRange [0, 1]', () => {
    const meta = TARGET_META.blockMix;
    expect(meta.kind).toBe('continuous');
    expect(meta.paramRange).toEqual({ min: 0, max: 1 });
    expect(meta.paramLabel).toBe('Mix');
  });

  it('declares blockBalance as continuous with paramRange [-1, 1]', () => {
    const meta = TARGET_META.blockBalance;
    expect(meta.kind).toBe('continuous');
    expect(meta.paramRange).toEqual({ min: -1, max: 1 });
  });

  it('declares blockLevel as continuous with paramRange [-60, 12]', () => {
    const meta = TARGET_META.blockLevel;
    expect(meta.kind).toBe('continuous');
    expect(meta.paramRange).toEqual({ min: -60, max: 12 });
  });

  it('declares non-shaping targets as kind=none', () => {
    expect(TARGET_META.sceneSwitch.kind).toBe('none');
    expect(TARGET_META.presetChange.kind).toBe('none');
  });

  it('formats and parses Mix values round-trip within rounding tolerance', () => {
    const meta = TARGET_META.blockMix;
    const formatted = meta.paramFormat!(0.55);
    expect(formatted).toBe('55%');
    expect(meta.paramParse!(formatted)).toBeCloseTo(0.55, 2);
  });

  it('formats and parses Level (dB) values round-trip', () => {
    const meta = TARGET_META.blockLevel;
    expect(meta.paramFormat!(-12.5)).toContain('-12.5');
    expect(meta.paramParse!('-12.5 dB')).toBeCloseTo(-12.5, 2);
  });

  it('blockMix declares input bounds 0..100 step 0.5 with suffix %', () => {
    const meta = TARGET_META.blockMix;
    expect(meta.paramInputMin).toBe(0);
    expect(meta.paramInputMax).toBe(100);
    expect(meta.paramInputStep).toBe(0.5);
    expect(meta.paramSuffix).toBe('%');
  });

  it('blockMix paramToDisplay/paramFromDisplay round-trip', () => {
    const meta = TARGET_META.blockMix;
    expect(meta.paramToDisplay!(0.55)).toBeCloseTo(55, 6);
    expect(meta.paramFromDisplay!(55)).toBeCloseTo(0.55, 6);
  });

  it('blockBalance has no suffix and 1:1 step', () => {
    const meta = TARGET_META.blockBalance;
    expect(meta.paramInputMin).toBe(-100);
    expect(meta.paramInputMax).toBe(100);
    expect(meta.paramInputStep).toBe(1);
    expect(meta.paramSuffix).toBeUndefined();
  });

  it('blockLevel suffix dB; display is identity (no scale)', () => {
    const meta = TARGET_META.blockLevel;
    expect(meta.paramSuffix).toBe('dB');
    expect(meta.paramToDisplay!(-12.5)).toBe(-12.5);
    expect(meta.paramFromDisplay!(-12.5)).toBe(-12.5);
  });

  it('declares blockBypass as binary with OFF/ON labels', () => {
    const meta = TARGET_META.blockBypass;
    expect(meta.kind).toBe('binary');
    expect(meta.binaryLabels).toEqual({ off: 'OFF', on: 'ON' });
    expect(meta.binaryHelpTemplate?.(64)).toMatch(/Block turns ON when CC ≥ 64/);
  });

  it('declares tunerToggle as binary with OFF/ON labels', () => {
    expect(TARGET_META.tunerToggle.kind).toBe('binary');
    expect(TARGET_META.tunerToggle.binaryLabels).toEqual({ off: 'OFF', on: 'ON' });
  });

  it('declares blockState as binary with IGNORED/RECALL labels', () => {
    const meta = TARGET_META.blockState;
    expect(meta.kind).toBe('binary');
    expect(meta.binaryLabels).toEqual({ off: 'IGNORED', on: 'RECALL' });
    expect(meta.binaryHelpTemplate?.(80)).toMatch(/State recalled when CC ≥ 80/);
  });
});
