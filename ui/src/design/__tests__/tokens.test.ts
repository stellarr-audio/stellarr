import { describe, it, expect, beforeEach } from 'vitest';
import '../tokens.css';

/**
 * JSDOM's `getComputedStyle` returns the raw declared value of a CSS custom
 * property — it does NOT resolve `var()` references. This helper walks one
 * level of indirection so tests can assert against the final value while
 * keeping the production CSS architecture (ramp tokens -> semantic aliases
 * via var()) intact.
 */
function getVar(name: string): string {
  const root = document.documentElement;
  const raw = getComputedStyle(root).getPropertyValue(name).trim();
  const match = raw.match(/^var\((--[a-z0-9-]+)\)$/i);
  if (match) {
    return getComputedStyle(root).getPropertyValue(match[1]).trim();
  }
  return raw;
}

describe('typography tokens', () => {
  beforeEach(() => {
    document.documentElement.removeAttribute('data-theme');
  });

  it('exposes --text-xs as 13px', () => {
    expect(getVar('--text-xs')).toBe('13px');
  });

  it('exposes --text-base as 15px', () => {
    expect(getVar('--text-base')).toBe('15px');
  });

  it('exposes --text-base-weight as 400', () => {
    expect(getVar('--text-base-weight')).toBe('400');
  });

  it('exposes --text-base-strong-weight as 600', () => {
    expect(getVar('--text-base-strong-weight')).toBe('600');
  });

  it('exposes --text-xs-weight as 500', () => {
    expect(getVar('--text-xs-weight')).toBe('500');
  });
});

describe('dimension tokens', () => {
  it('defines row-standard dimensions', () => {
    expect(getVar('--row-gap')).toBe('24px');
    expect(getVar('--row-actions-width')).toBe('320px');
  });
});

describe('palette tokens — light theme', () => {
  beforeEach(() => {
    document.documentElement.setAttribute('data-theme', 'light');
  });

  it('exposes --accent as #c026d3 (Orchid-600)', () => {
    expect(getVar('--accent')).toBe('#c026d3');
  });

  it('exposes --accent-text as #a21caf (Orchid-700)', () => {
    expect(getVar('--accent-text')).toBe('#a21caf');
  });

  it('exposes --secondary as #f59e0b (Amber-500)', () => {
    expect(getVar('--secondary')).toBe('#f59e0b');
  });

  it('exposes --success as #10b981 (Emerald-500)', () => {
    expect(getVar('--success')).toBe('#10b981');
  });

  it('exposes --danger as #e11d48 (Rose-600)', () => {
    expect(getVar('--danger')).toBe('#e11d48');
  });

  it('has --bg on grey-100 (playground surface)', () => {
    expect(getVar('--bg')).toBe('#f1f3f5');
  });

  it('has --chrome on grey-0 (header + footer surface)', () => {
    expect(getVar('--chrome')).toBe('#ffffff');
  });

  it('has --text on grey-900', () => {
    expect(getVar('--text')).toBe('#1c1e22');
  });
});

describe('palette tokens — dark theme', () => {
  beforeEach(() => {
    document.documentElement.setAttribute('data-theme', 'dark');
  });

  it('exposes --accent as #d946ef (Orchid-500 for dark)', () => {
    expect(getVar('--accent')).toBe('#d946ef');
  });

  it('exposes --accent-text as #f0abfc (Orchid-300)', () => {
    expect(getVar('--accent-text')).toBe('#f0abfc');
  });

  it('exposes --secondary as #fbbf24 (Amber-400)', () => {
    expect(getVar('--secondary')).toBe('#fbbf24');
  });

  it('has --text as #e8f1fc (off-white)', () => {
    expect(getVar('--text')).toBe('#e8f1fc');
  });

  it('has --chrome on grey-dark-100 (header + footer surface over gradient bg)', () => {
    expect(getVar('--chrome')).toBe('#0f1a2e');
  });
});
