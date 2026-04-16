import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import { useThemeStore, resolveTheme } from '../theme';

describe('theme store', () => {
  beforeEach(() => {
    localStorage.clear();
    useThemeStore.setState({ theme: 'system' });
  });

  it('defaults to system', () => {
    expect(useThemeStore.getState().theme).toBe('system');
  });

  it('setTheme updates state', () => {
    useThemeStore.getState().setTheme('light');
    expect(useThemeStore.getState().theme).toBe('light');
  });

  it('toggleTheme cycles system → light → dark → system', () => {
    useThemeStore.setState({ theme: 'system' });
    useThemeStore.getState().toggleTheme();
    expect(useThemeStore.getState().theme).toBe('light');

    useThemeStore.getState().toggleTheme();
    expect(useThemeStore.getState().theme).toBe('dark');

    useThemeStore.getState().toggleTheme();
    expect(useThemeStore.getState().theme).toBe('system');
  });

  it('persists to localStorage under "stellarr.theme"', () => {
    useThemeStore.getState().setTheme('dark');
    const stored = JSON.parse(localStorage.getItem('stellarr.theme')!);
    expect(stored.state.theme).toBe('dark');
  });
});

describe('resolveTheme', () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it('returns the exact value for "light" or "dark"', () => {
    expect(resolveTheme('light')).toBe('light');
    expect(resolveTheme('dark')).toBe('dark');
  });

  it('follows matchMedia for "system"', () => {
    const mql = (matches: boolean) =>
      ({
        matches,
        media: '(prefers-color-scheme: dark)',
        addEventListener: vi.fn(),
        removeEventListener: vi.fn(),
      }) as unknown as MediaQueryList;

    vi.stubGlobal('matchMedia', (q: string) => (q.includes('dark') ? mql(true) : mql(false)));
    expect(resolveTheme('system')).toBe('dark');

    vi.stubGlobal('matchMedia', (q: string) => (q.includes('dark') ? mql(false) : mql(true)));
    expect(resolveTheme('system')).toBe('light');
  });
});
