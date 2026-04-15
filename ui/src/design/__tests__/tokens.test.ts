import { describe, it, expect, beforeEach } from 'vitest';
import '../tokens.css';

describe('typography tokens', () => {
  beforeEach(() => {
    document.documentElement.removeAttribute('data-theme');
  });

  it('exposes --text-xs as 13px', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text-xs').trim();
    expect(v).toBe('13px');
  });

  it('exposes --text-base as 15px', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text-base').trim();
    expect(v).toBe('15px');
  });

  it('exposes --text-base-weight as 400', () => {
    const v = getComputedStyle(document.documentElement)
      .getPropertyValue('--text-base-weight')
      .trim();
    expect(v).toBe('400');
  });

  it('exposes --text-base-strong-weight as 600', () => {
    const v = getComputedStyle(document.documentElement)
      .getPropertyValue('--text-base-strong-weight')
      .trim();
    expect(v).toBe('600');
  });

  it('exposes --text-xs-weight as 500', () => {
    const v = getComputedStyle(document.documentElement)
      .getPropertyValue('--text-xs-weight')
      .trim();
    expect(v).toBe('500');
  });
});
