import { describe, it, expect } from 'vitest';
import { colors } from '../colors';

describe('colors module', () => {
  it('returns CSS variable references, not literal values', () => {
    // every value should be a `var(--…)` string so it follows the theme
    for (const v of Object.values(colors)) {
      expect(v).toMatch(/^var\(--[a-z-]+\)$/);
    }
  });

  it('preserves the public API (all original keys)', () => {
    expect(colors).toHaveProperty('bg');
    expect(colors).toHaveProperty('surface');
    expect(colors).toHaveProperty('cell');
    expect(colors).toHaveProperty('cellHover');
    expect(colors).toHaveProperty('border');
    expect(colors).toHaveProperty('primary');
    expect(colors).toHaveProperty('secondary');
    expect(colors).toHaveProperty('text');
    expect(colors).toHaveProperty('muted');
    expect(colors).toHaveProperty('green');
    expect(colors).toHaveProperty('blockBg');
    expect(colors).toHaveProperty('blockBorder');
    expect(colors).toHaveProperty('portColor');
    expect(colors).toHaveProperty('connectionLine');
    expect(colors).toHaveProperty('dropdownBg');
    expect(colors).toHaveProperty('danger');
    expect(colors).toHaveProperty('warning');
    expect(colors).toHaveProperty('dangerBright');
  });
});
