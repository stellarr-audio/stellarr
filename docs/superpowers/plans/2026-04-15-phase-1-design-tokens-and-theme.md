# Phase 1: Design tokens and theme switching — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Translate the locked Phase 0 tokens into working CSS variables with a light/dark theme switching mechanism, and establish the Vitest-based UI test infrastructure we've been missing.

**Architecture:** Introduce `ui/src/design/tokens.css` as the single source of truth for all design tokens. Both themes live in the same stylesheet, scoped by `[data-theme="light"]` and `[data-theme="dark"]`. A Zustand slice manages `'light' | 'dark' | 'system'` state with localStorage persistence, and an effect syncs the resolved theme to `document.documentElement.dataset.theme`. The existing `variables.css` is reworked so its legacy `--color-*` names become aliases pointing at the new tokens — no sweeping rename of 37 CSS modules needed. Vitest + JSDOM is added as the project's first UI test runner.

**Tech Stack:** CSS custom properties, Zustand (with persist middleware), Vitest, JSDOM, `@testing-library/react`, `@testing-library/jest-dom`.

**Scope reminder:** This plan does NOT build new primitive components (Button, Select, etc.). Phase 2 (header + footer) will surface exactly which primitives are needed and they'll be built there on a real consumer. Per the spec, speculatively building a library is out of scope.

**Visual impact warning:** The existing app uses `--color-primary: #ff2d7b` (hot pink) as its accent. This plan replaces it with the locked Orchid brand (`#c026d3`). After Phase 1 merges, the running app will look Orchid-themed — expected and intended.

---

## File structure

**Created:**
- `ui/src/design/tokens.css` — the locked tokens (palette, typography) for both themes
- `ui/src/design/index.ts` — barrel export for tokens-as-JS if/when needed
- `ui/src/store/theme.ts` — Zustand slice for `'light' | 'dark' | 'system'` plus resolved value
- `ui/src/hooks/useSyncTheme.ts` — effect hook that mirrors theme state to `document.documentElement`
- `ui/vitest.config.ts` — Vitest configuration (separate from vite.config.ts to keep concerns split)
- `ui/src/__tests__/setup.ts` — shared test setup (jest-dom matchers)
- `ui/src/design/__tests__/tokens.test.ts` — tests that tokens resolve correctly per theme
- `ui/src/store/__tests__/theme.test.ts` — tests for the theme slice
- `ui/src/hooks/__tests__/useSyncTheme.test.tsx` — tests for the document sync

**Modified:**
- `ui/package.json` — add Vitest, JSDOM, testing-library devDeps + `test` script
- `ui/src/main.tsx` — import `tokens.css`, call `useSyncTheme`
- `ui/src/styles/variables.css` — reduced to a compatibility layer (aliases to new tokens)
- `ui/src/components/common/colors.ts` — re-exported as thin `var(--…)` wrappers (no values duplicated)
- `ui/src/App.tsx` — invoke `useSyncTheme()` at the top

---

### Task 1: Add Vitest + JSDOM + testing-library

**Files:**
- Modify: `ui/package.json`
- Create: `ui/vitest.config.ts`
- Create: `ui/src/__tests__/setup.ts`
- Create: `ui/src/__tests__/smoke.test.ts`

- [ ] **Step 1: Install dependencies**

Run in `ui/`:

```bash
cd ui && npm install --save-dev vitest@latest jsdom@latest @testing-library/react@latest @testing-library/dom@latest @testing-library/jest-dom@latest
```

Expected: `package.json` updated; `node_modules/` includes the five new packages.

- [ ] **Step 2: Create `ui/vitest.config.ts`**

```ts
import { defineConfig } from 'vitest/config';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  test: {
    environment: 'jsdom',
    globals: true,
    setupFiles: ['./src/__tests__/setup.ts'],
    css: true, // resolve CSS imports so tokens get applied in JSDOM
  },
});
```

- [ ] **Step 3: Create `ui/src/__tests__/setup.ts`**

```ts
import '@testing-library/jest-dom/vitest';
```

- [ ] **Step 4: Add `test` script to `ui/package.json`**

Modify the `"scripts"` block:

```json
"scripts": {
  "dev": "vite",
  "build": "tsc && vite build",
  "preview": "vite preview",
  "format": "prettier --write \"src/**/*.{ts,tsx}\"",
  "test": "vitest run",
  "test:watch": "vitest"
}
```

- [ ] **Step 5: Write a smoke test at `ui/src/__tests__/smoke.test.ts`**

```ts
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
```

- [ ] **Step 6: Run the smoke test**

```bash
cd ui && npm run test
```

Expected: 2 passed, 0 failed.

- [ ] **Step 7: Commit**

```bash
git add ui/package.json ui/package-lock.json ui/vitest.config.ts ui/src/__tests__/setup.ts ui/src/__tests__/smoke.test.ts
git commit -m "test(ui): add Vitest + JSDOM + testing-library setup"
```

---

### Task 2: Create typography tokens

**Files:**
- Create: `ui/src/design/tokens.css`
- Create: `ui/src/design/__tests__/tokens.test.ts`
- Modify: `ui/src/main.tsx`

- [ ] **Step 1: Write the failing test at `ui/src/design/__tests__/tokens.test.ts`**

```ts
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
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text-base-weight').trim();
    expect(v).toBe('400');
  });

  it('exposes --text-base-strong-weight as 600', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text-base-strong-weight').trim();
    expect(v).toBe('600');
  });

  it('exposes --text-xs-weight as 500', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text-xs-weight').trim();
    expect(v).toBe('500');
  });
});
```

- [ ] **Step 2: Run the test to verify it fails**

```bash
cd ui && npm run test -- tokens
```

Expected: FAIL — `--text-xs` resolves to empty string (tokens.css not created yet).

- [ ] **Step 3: Create `ui/src/design/tokens.css` with typography**

```css
/*
 * Stellarr design tokens.
 * Locked in Phase 0 — see docs/superpowers/specs/2026-04-15-phase-0-design-direction.md
 * Edit here only; all components consume these tokens.
 */

:root {
  /* Typography — identical across themes */
  --text-xs: 13px;
  --text-xs-weight: 500;

  --text-base: 15px;
  --text-base-weight: 400;
  --text-base-strong-weight: 600;

  /* --text-display reserved — concrete value TBD when Tuner screen is designed */

  --line-height-body: 1.4;
  --line-height-heading: 1.2;
  --line-height-display: 1.0;

  --letter-spacing-default: 0.01em;
  --letter-spacing-label: 0.08em;
  --letter-spacing-tight: -0.01em;
}
```

- [ ] **Step 4: Import `tokens.css` in `ui/src/main.tsx`**

Find the existing CSS imports near the top of `ui/src/main.tsx` and add the tokens import BEFORE `variables.css`:

```ts
import './design/tokens.css';
import './styles/variables.css';
import './assets/fonts/fonts.css';
// … rest of imports unchanged
```

(Order matters — tokens.css must load first so variables.css can reference it.)

- [ ] **Step 5: Run tests to verify they pass**

```bash
cd ui && npm run test -- tokens
```

Expected: 5 passed.

- [ ] **Step 6: Commit**

```bash
git add ui/src/design/tokens.css ui/src/design/__tests__/tokens.test.ts ui/src/main.tsx
git commit -m "feat(ui): add typography design tokens"
```

---

### Task 3: Add palette tokens for both themes

**Files:**
- Modify: `ui/src/design/tokens.css`
- Modify: `ui/src/design/__tests__/tokens.test.ts`

- [ ] **Step 1: Extend the failing test**

Append to `ui/src/design/__tests__/tokens.test.ts`:

```ts
describe('palette tokens — light theme', () => {
  beforeEach(() => {
    document.documentElement.setAttribute('data-theme', 'light');
  });

  it('exposes --accent as #c026d3 (Orchid-600)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim();
    expect(v).toBe('#c026d3');
  });

  it('exposes --accent-text as #a21caf (Orchid-700)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--accent-text').trim();
    expect(v).toBe('#a21caf');
  });

  it('exposes --secondary as #f59e0b (Amber-500)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--secondary').trim();
    expect(v).toBe('#f59e0b');
  });

  it('exposes --success as #10b981 (Emerald-500)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--success').trim();
    expect(v).toBe('#10b981');
  });

  it('exposes --danger as #e11d48 (Rose-600)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--danger').trim();
    expect(v).toBe('#e11d48');
  });

  it('has --bg on grey-50', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--bg').trim();
    expect(v).toBe('#fafbfc');
  });

  it('has --text on grey-900', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text').trim();
    expect(v).toBe('#1c1e22');
  });
});

describe('palette tokens — dark theme', () => {
  beforeEach(() => {
    document.documentElement.setAttribute('data-theme', 'dark');
  });

  it('exposes --accent as #d946ef (Orchid-500 for dark)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim();
    expect(v).toBe('#d946ef');
  });

  it('exposes --accent-text as #f0abfc (Orchid-300)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--accent-text').trim();
    expect(v).toBe('#f0abfc');
  });

  it('exposes --secondary as #fbbf24 (Amber-400)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--secondary').trim();
    expect(v).toBe('#fbbf24');
  });

  it('has --text as #e8f1fc (off-white)', () => {
    const v = getComputedStyle(document.documentElement).getPropertyValue('--text').trim();
    expect(v).toBe('#e8f1fc');
  });
});
```

- [ ] **Step 2: Run tests to verify they fail**

```bash
cd ui && npm run test -- tokens
```

Expected: the new 11 palette-related tests FAIL (tokens don't exist yet).

- [ ] **Step 3: Extend `ui/src/design/tokens.css`** with palette + theme scoping

Append to `tokens.css` (below the typography block):

```css
/* === Theme-agnostic palette ramps =========================================
 * These are the raw colour values. Semantic aliases below pick from them
 * per theme.
 */
:root {
  /* Grey */
  --grey-0:   #ffffff;
  --grey-50:  #fafbfc;
  --grey-100: #f1f3f5;
  --grey-200: #e5e7eb;
  --grey-300: #d1d5db;
  --grey-400: #9ca3af;
  --grey-500: #6b7280;
  --grey-600: #4b5563;
  --grey-700: #374151;
  --grey-900: #1c1e22;

  /* Grey — dark-theme aliases (same ramp, different intent) */
  --grey-dark-0:   #050510;
  --grey-dark-50:  #0a0a1e;
  --grey-dark-100: #0f1a2e;
  --grey-dark-200: #1a2438;
  --grey-dark-300: #2a3348;
  --grey-dark-400: #9ba3b4;
  --grey-dark-500: #6b7e96;
  --grey-dark-600: #8a9bb0;
  --grey-dark-700: #c8d4e6;
  --grey-dark-900: #e8f1fc;

  /* Orchid (primary brand) */
  --orchid-50:  #fdf4ff;
  --orchid-100: #fae8ff;
  --orchid-200: #f5d0fe;
  --orchid-300: #f0abfc;
  --orchid-400: #e879f9;
  --orchid-500: #d946ef;
  --orchid-600: #c026d3;
  --orchid-700: #a21caf;
  --orchid-800: #86198f;
  --orchid-900: #701a75;

  /* Amber (secondary) */
  --amber-400: #fbbf24;
  --amber-500: #f59e0b;

  /* Emerald (success) */
  --emerald-500: #10b981;

  /* Rose (danger) */
  --rose-600: #e11d48;
}

/* === Light theme ========================================================== */
:root,
[data-theme="light"] {
  --accent:       var(--orchid-600);       /* #c026d3 */
  --accent-text:  var(--orchid-700);       /* #a21caf */
  --secondary:    var(--amber-500);        /* #f59e0b */
  --success:      var(--emerald-500);      /* #10b981 */
  --danger:       var(--rose-600);         /* #e11d48 */

  --bg:           var(--grey-50);          /* #fafbfc */
  --surface:      var(--grey-0);           /* #ffffff */
  --border:       var(--grey-200);         /* #e5e7eb */
  --text:         var(--grey-900);         /* #1c1e22 */
  --text-muted:   var(--grey-500);         /* #6b7280 */
  --text-subtle:  var(--grey-400);         /* #9ca3af */
}

/* === Dark theme =========================================================== */
[data-theme="dark"] {
  --accent:       var(--orchid-500);       /* #d946ef — brighter, carries glow */
  --accent-text:  var(--orchid-300);       /* #f0abfc */
  --secondary:    var(--amber-400);        /* #fbbf24 */
  --success:      var(--emerald-500);      /* same */
  --danger:       var(--rose-600);         /* same */

  /* Dark --bg is a radial gradient per the Phase 0 spec. Apply via
     `background: var(--bg)` on the top-level container (not background-color). */
  --bg:           radial-gradient(ellipse 90% 70% at 40% -10%, #1e1a3e 0%, #0a0a1e 45%, var(--grey-dark-0) 100%);
  --surface:      rgba(255,255,255,0.03);
  --border:       rgba(255,255,255,0.06);
  --text:         var(--grey-dark-900);    /* #e8f1fc */
  --text-muted:   var(--grey-dark-500);    /* #6b7e96 */
  --text-subtle:  var(--grey-dark-400);    /* #9ba3b4 */
}
```

Note: `:root, [data-theme="light"]` defaults to light so that a document with no attribute set still renders correctly (rather than being unthemed).

- [ ] **Step 4: Run tests to verify they pass**

```bash
cd ui && npm run test -- tokens
```

Expected: all 16 token tests pass.

- [ ] **Step 5: Commit**

```bash
git add ui/src/design/tokens.css ui/src/design/__tests__/tokens.test.ts
git commit -m "feat(ui): add palette tokens with light/dark theming"
```

---

### Task 4: Refactor `variables.css` into a compatibility layer

**Files:**
- Modify: `ui/src/styles/variables.css`

**Why:** 37 CSS modules reference `--color-bg`, `--color-primary`, etc. Rather than rename them all, redefine those names as aliases pointing at the new tokens. Zero component changes needed; the whole app picks up the new palette automatically.

- [ ] **Step 1: Read the current variables.css to understand what's there**

```bash
cat ui/src/styles/variables.css
```

Note the full list of `--color-*` tokens — you'll re-alias every one of them.

- [ ] **Step 2: Replace `ui/src/styles/variables.css` with the compatibility layer**

Full replacement content:

```css
/*
 * Legacy token compatibility layer.
 *
 * All real tokens now live in `../design/tokens.css`. These --color-* names
 * are retained as aliases so existing CSS modules (37 files) keep working.
 * Prefer using the new tokens (`var(--accent)`, `var(--bg)` etc.) in new
 * code — these aliases may be removed in a later phase.
 */

:root {
  --color-bg:         var(--bg);
  --color-surface:    var(--surface);
  --color-cell:       var(--surface);
  --color-text:       var(--text);
  --color-muted:      var(--text-muted);
  --color-border:     var(--border);

  --color-primary:    var(--accent);
  --color-secondary:  var(--secondary);
  --color-danger:     var(--danger);
  --color-warning:    var(--secondary);  /* warning folds into secondary/amber */

  --color-connection: var(--text-muted);
  --color-block-default:  var(--surface);
  --color-block-selected: var(--accent);

  --color-dropdown-bg: var(--surface);
}
```

Replace every `--color-*` name that existed in the original file — if Step 1 showed names not listed above, add them with a sensible alias.

- [ ] **Step 3: Build the UI and start the dev server for a visual smoke test**

```bash
cd ui && npm run build 2>&1 | tail -10
```

Expected: build succeeds with no errors.

- [ ] **Step 4: Commit**

```bash
git add ui/src/styles/variables.css
git commit -m "refactor(ui): alias legacy --color-* tokens onto new design tokens"
```

---

### Task 5: Theme Zustand slice with light/dark/system + localStorage

**Files:**
- Create: `ui/src/store/theme.ts`
- Create: `ui/src/store/__tests__/theme.test.ts`

- [ ] **Step 1: Write the failing tests at `ui/src/store/__tests__/theme.test.ts`**

```ts
import { describe, it, expect, beforeEach, vi } from 'vitest';
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
  it('returns the exact value for "light" or "dark"', () => {
    expect(resolveTheme('light')).toBe('light');
    expect(resolveTheme('dark')).toBe('dark');
  });

  it('follows matchMedia for "system"', () => {
    const mql = (matches: boolean) =>
      ({ matches, media: '(prefers-color-scheme: dark)', addEventListener: vi.fn(), removeEventListener: vi.fn() }) as unknown as MediaQueryList;

    vi.stubGlobal('matchMedia', (q: string) => (q.includes('dark') ? mql(true) : mql(false)));
    expect(resolveTheme('system')).toBe('dark');

    vi.stubGlobal('matchMedia', (q: string) => (q.includes('dark') ? mql(false) : mql(true)));
    expect(resolveTheme('system')).toBe('light');
  });
});
```

- [ ] **Step 2: Run to verify it fails**

```bash
cd ui && npm run test -- theme
```

Expected: import of `'../theme'` fails — module doesn't exist yet.

- [ ] **Step 3: Implement `ui/src/store/theme.ts`**

```ts
import { create } from 'zustand';
import { persist, createJSONStorage } from 'zustand/middleware';

export type ThemePreference = 'light' | 'dark' | 'system';
export type ResolvedTheme = 'light' | 'dark';

interface ThemeState {
  theme: ThemePreference;
  setTheme: (t: ThemePreference) => void;
  toggleTheme: () => void;
}

const order: ThemePreference[] = ['system', 'light', 'dark'];

export const useThemeStore = create<ThemeState>()(
  persist(
    (set, get) => ({
      theme: 'system',
      setTheme: (t) => set({ theme: t }),
      toggleTheme: () => {
        const i = order.indexOf(get().theme);
        set({ theme: order[(i + 1) % order.length] });
      },
    }),
    {
      name: 'stellarr.theme',
      storage: createJSONStorage(() => localStorage),
    },
  ),
);

export function resolveTheme(pref: ThemePreference): ResolvedTheme {
  if (pref === 'light' || pref === 'dark') return pref;
  if (typeof window === 'undefined' || !window.matchMedia) return 'dark';
  return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}
```

- [ ] **Step 4: Run to verify tests pass**

```bash
cd ui && npm run test -- theme
```

Expected: 6 passed.

- [ ] **Step 5: Commit**

```bash
git add ui/src/store/theme.ts ui/src/store/__tests__/theme.test.ts
git commit -m "feat(ui): add theme store with light/dark/system + persistence"
```

---

### Task 6: `useSyncTheme` hook mirrors theme state onto `document.documentElement`

**Files:**
- Create: `ui/src/hooks/useSyncTheme.ts`
- Create: `ui/src/hooks/__tests__/useSyncTheme.test.tsx`
- Modify: `ui/src/App.tsx`

- [ ] **Step 1: Write the failing test at `ui/src/hooks/__tests__/useSyncTheme.test.tsx`**

```tsx
import { describe, it, expect, beforeEach, vi } from 'vitest';
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
    mockMatchMedia(true); // default: system prefers dark
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
});
```

- [ ] **Step 2: Run to verify it fails**

```bash
cd ui && npm run test -- useSyncTheme
```

Expected: import of `'../useSyncTheme'` fails.

- [ ] **Step 3: Implement `ui/src/hooks/useSyncTheme.ts`**

```ts
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
```

- [ ] **Step 4: Run to verify tests pass**

```bash
cd ui && npm run test -- useSyncTheme
```

Expected: 5 passed.

- [ ] **Step 5: Wire the hook into `ui/src/App.tsx`**

Open `ui/src/App.tsx`. Add an import at the top with the other imports:

```tsx
import { useSyncTheme } from './hooks/useSyncTheme';
```

Inside the `App` component (as the very first line of the function body):

```tsx
export function App() {
  useSyncTheme();

  // … existing body unchanged
```

- [ ] **Step 6: Verify the build passes**

```bash
cd ui && npm run build 2>&1 | tail -5
```

Expected: build succeeds.

- [ ] **Step 7: Commit**

```bash
git add ui/src/hooks/useSyncTheme.ts ui/src/hooks/__tests__/useSyncTheme.test.tsx ui/src/App.tsx
git commit -m "feat(ui): sync theme preference to document data-theme attribute"
```

---

### Task 7: Consolidate `colors.ts` to re-export CSS variable names

**Files:**
- Modify: `ui/src/components/common/colors.ts`
- Create: `ui/src/components/common/__tests__/colors.test.ts`

**Why:** The current `colors.ts` duplicates the colour values as a JS object (camelCase). We want a single source of truth in `tokens.css`. The JS module is still useful (e.g. passing colour strings to canvas/SVG contexts), but should reference the CSS variable — never hardcode values.

- [ ] **Step 1: Read the current `colors.ts` to see what shape it exports**

```bash
cat ui/src/components/common/colors.ts
```

Note every property name so the replacement preserves every key.

- [ ] **Step 2: Write a failing test at `ui/src/components/common/__tests__/colors.test.ts`**

```ts
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
    // adjust this list to match whatever the original colors.ts exported
    expect(colors).toHaveProperty('bg');
    expect(colors).toHaveProperty('accent');
    expect(colors).toHaveProperty('border');
    expect(colors).toHaveProperty('text');
  });
});
```

Adjust the second `it` block's expected keys to match whatever Step 1 showed.

- [ ] **Step 3: Rewrite `ui/src/components/common/colors.ts`**

```ts
/**
 * Re-exports design tokens as CSS variable references for use in JS/TS
 * (e.g. inline styles, canvas / SVG fill attributes).
 *
 * The *values* live in `ui/src/design/tokens.css`. This module only provides
 * the `var(--…)` strings. Do NOT hardcode hex colours here — use tokens.
 */

export const colors = {
  // Surfaces
  bg:          'var(--bg)',
  surface:     'var(--surface)',
  border:      'var(--border)',

  // Text
  text:        'var(--text)',
  textMuted:   'var(--text-muted)',
  textSubtle:  'var(--text-subtle)',

  // Accents
  accent:      'var(--accent)',
  accentText:  'var(--accent-text)',
  secondary:   'var(--secondary)',

  // Semantic
  success:     'var(--success)',
  danger:      'var(--danger)',
} as const;

export type ColorToken = keyof typeof colors;
```

If the original `colors.ts` had other keys (e.g. `cell`, `connection`, `dropdownBg`), add them with appropriate `var(--…)` mappings (`--surface`, `--text-muted`, `--surface` respectively). Use the legacy `variables.css` aliases from Task 4 as a guide.

- [ ] **Step 4: Run tests**

```bash
cd ui && npm run test -- colors
```

Expected: 2 passed.

- [ ] **Step 5: Build to verify no callers are broken**

```bash
cd ui && npm run build 2>&1 | tail -10
```

Expected: type check passes, build completes.

If TypeScript errors appear (e.g. a caller referenced `colors.someOldKey` that you didn't include), add that key back to `colors.ts` with the right `var(--…)` mapping.

- [ ] **Step 6: Commit**

```bash
git add ui/src/components/common/colors.ts ui/src/components/common/__tests__/colors.test.ts
git commit -m "refactor(ui): consolidate colors.ts to re-export CSS variables only"
```

---

### Task 8: Manual smoke test — verify both themes render

**Files:** none (verification only)

- [ ] **Step 1: Start the dev server**

```bash
cd ui && npm run dev
```

Open the URL printed in the terminal in a browser.

- [ ] **Step 2: Verify dark mode renders correctly**

In the browser DevTools, confirm:
- `<html>` has `data-theme="dark"` (if your OS is set to dark mode) or `data-theme="light"` (if your OS is light)
- The app's accent colour (on anything currently pink) is now **Orchid** — i.e. a warm purple
- No layout breakage — the app is functional

- [ ] **Step 3: Force the other theme via DevTools and re-verify**

In DevTools, manually edit `<html data-theme="light">` (or `"dark"`) to switch. Confirm the app renders in the other theme without visual artefacts.

- [ ] **Step 4: Verify localStorage persistence**

In the browser console:

```js
localStorage.getItem('stellarr.theme')
```

Expected: a JSON string containing `"theme":"system"` (or whatever was last set).

- [ ] **Step 5: Run the full test suite one more time**

```bash
cd ui && npm run test
```

Expected: all tests pass — typography (5) + palette (11) + theme store (6) + useSyncTheme (5) + colors (2) + smoke (2) = 31 passed.

- [ ] **Step 6: TypeScript check**

```bash
cd ui && npx tsc --noEmit
```

Expected: no errors.

- [ ] **Step 7: Final commit (if any cleanup needed)**

If you found and fixed any issues in the smoke test:

```bash
git add -p  # stage relevant hunks
git commit -m "fix(ui): address Phase 1 smoke-test findings"
```

Otherwise, skip this step.

---

## What this plan does NOT do (out of scope for Phase 1)

- Build primitive components (Button, Select, RadioGroup, Dropdown). These will be added in Phase 2 when the header/footer migration surfaces exactly what shapes are needed.
- Add a visible theme-switch toggle in the UI. The store and hook exist; the UI control comes in Phase 2 (in the Settings tab) when we rebuild that chrome.
- Rename the 37 legacy `--color-*` module references to the new names. Kept as-is via the compatibility layer in Task 4.
- Introduce spacing tokens (`--space-*`) or border-radius tokens. They'll be added as Phase 2 consumers need them.
- Update or remove the `fonts.css` import. Switzer stays as-is.

## Spec ↔ plan coverage

| Spec requirement | Covered in task |
|---|---|
| Orchid primary `#c026d3` / `#d946ef` | Task 3 |
| Amber secondary `#f59e0b` / `#fbbf24` | Task 3 |
| Emerald success `#10b981` | Task 3 |
| Rose danger `#e11d48` | Task 3 |
| 10-step grey ramp | Task 3 |
| Light + dark theme system | Task 3 + Task 6 |
| Theme-switch mechanism (light / dark / system) | Task 5 + Task 6 |
| Persistence across sessions | Task 5 (localStorage) |
| Switzer typography scale (13px / 15px / 15-bold) | Task 2 |
| Single source of truth for tokens | Task 2 + Task 3 + Task 4 + Task 7 |
| Test infrastructure (this plan needs it) | Task 1 |
