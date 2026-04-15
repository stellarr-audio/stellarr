import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest';
import { renderHook, act } from '@testing-library/react';
import { useSyncTheme } from '../useSyncTheme';
import { useThemeStore } from '../../store/theme';

function mockMatchMedia(dark: boolean) {
  vi.stubGlobal('matchMedia', (q: string) => ({
    matches: q.includes('dark') ? dark : !dark,
    media: q,
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
  }));
}

describe('useSyncTheme', () => {
  beforeEach(() => {
    document.documentElement.removeAttribute('data-theme');
    useThemeStore.setState({ theme: 'system' });
    mockMatchMedia(true);
  });

  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it('sets data-theme="dark" when theme is "dark"', () => {
    useThemeStore.setState({ theme: 'dark' });
    renderHook(() => useSyncTheme());
    expect(document.documentElement.dataset.theme).toBe('dark');
  });

  it('sets data-theme="light" when theme is "light"', () => {
    useThemeStore.setState({ theme: 'light' });
    renderHook(() => useSyncTheme());
    expect(document.documentElement.dataset.theme).toBe('light');
  });

  it('resolves "system" to OS preference (dark)', () => {
    mockMatchMedia(true);
    useThemeStore.setState({ theme: 'system' });
    renderHook(() => useSyncTheme());
    expect(document.documentElement.dataset.theme).toBe('dark');
  });

  it('resolves "system" to OS preference (light)', () => {
    mockMatchMedia(false);
    useThemeStore.setState({ theme: 'system' });
    renderHook(() => useSyncTheme());
    expect(document.documentElement.dataset.theme).toBe('light');
  });

  it('updates data-theme when preference changes', () => {
    renderHook(() => useSyncTheme());
    act(() => useThemeStore.getState().setTheme('light'));
    expect(document.documentElement.dataset.theme).toBe('light');

    act(() => useThemeStore.getState().setTheme('dark'));
    expect(document.documentElement.dataset.theme).toBe('dark');
  });

  it('registers and cleans up prefers-color-scheme listener when theme is "system"', () => {
    const addListener = vi.fn();
    const removeListener = vi.fn();
    vi.stubGlobal('matchMedia', (q: string) => ({
      matches: q.includes('dark'),
      media: q,
      addEventListener: addListener,
      removeEventListener: removeListener,
    }));

    useThemeStore.setState({ theme: 'system' });
    const { unmount } = renderHook(() => useSyncTheme());

    // Listener should be attached
    expect(addListener).toHaveBeenCalledWith('change', expect.any(Function));

    // The registered callback, when invoked, should re-read OS preference
    // and update data-theme
    const callback = addListener.mock.calls[0]?.[1] as () => void;
    vi.stubGlobal('matchMedia', (q: string) => ({
      matches: !q.includes('dark'), // flip OS preference to light
      media: q,
      addEventListener: vi.fn(),
      removeEventListener: vi.fn(),
    }));
    act(() => callback());
    expect(document.documentElement.dataset.theme).toBe('light');

    // Unmount should remove the listener
    unmount();
    expect(removeListener).toHaveBeenCalledWith('change', callback);
  });
});
