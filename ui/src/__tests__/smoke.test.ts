import { describe, it, expect } from 'vitest';

describe('vitest setup', () => {
  it('runs a trivial assertion', () => {
    expect(1 + 1).toBe(2);
  });

  it('has access to jsdom', () => {
    const el = document.createElement('div');
    expect(el.tagName).toBe('DIV');
  });
});
