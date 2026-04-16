import { useEffect } from 'react';
import { useThemeStore, resolveTheme } from '../store/theme';

/**
 * Mirrors the theme-store preference onto `document.documentElement.dataset.theme`
 * so that CSS `[data-theme="..."]` selectors work. Also listens to the system
 * `prefers-color-scheme` when preference is `"system"`.
 *
 * Call once near the root of the React tree (e.g. inside App).
 */
export function useSyncTheme(): void {
  const theme = useThemeStore((s) => s.theme);

  useEffect(() => {
    const apply = () => {
      document.documentElement.dataset.theme = resolveTheme(theme);
    };

    apply();

    if (theme !== 'system') return;

    const mql = window.matchMedia?.('(prefers-color-scheme: dark)');
    if (!mql) return;

    mql.addEventListener('change', apply);
    return () => mql.removeEventListener('change', apply);
  }, [theme]);
}
