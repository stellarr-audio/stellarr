# Phase 0 · Design Direction

**Status:** approved 2026-04-15; living doc — token/primitive sections updated as Phases 1 & 2 ship
**Author:** brainstorm session (Claude + Rey)
**Scope:** whole-app visual redesign. Establishes the visual north star + the design-system contract (tokens + primitives). Does not specify how features behave.

## Context

Stellarr is planned for a whole-app UI redesign. To avoid the churn of building bespoke UI against the current design just to rip it out, we've decomposed the redesign into phases:

| Phase | Scope | Output |
|---|---|---|
| **0 · Design direction** *(this doc)* | Visual language — aesthetic, palette, typography | Locked tokens; mockups for reference |
| 1 · Design system | `ui/src/design/` tokens, theme CSS, theme-switch mechanism, primitive components | Code |
| 2 · Header + footer | Apply the design system to the header + footer chrome | Code |
| 3 · Canvas migration | Grid / ConnectionLayer → React Flow (or equivalent); dynamic grid sizing via pan/zoom | Code |
| 4 · Options panel + polish | Floating options panel on the new canvas; dialogs; motion polish | Code |

This doc closes Phase 0. Phases 1–4 will each get their own spec + plan when we reach them.

The dynamic-grid row/column feature originally proposed (append/remove rows/columns, grid centering) **disappears into Phase 3** — React Flow's pan/zoom + snap-to-grid gives us dynamic sizing for free, making the dedicated row/col UI unnecessary.

## Decisions locked in Phase 0

### 1. Aesthetic direction

A single design language that supports **light and dark themes**, derived from unifying:

- **"Flat Data" aesthetic** (FabFilter / iZotope / Ozone) — light, spacious, data-forward, crisp 1px borders, curves and meters as the hero visualisations
- **"Glassmorphic Modern" aesthetic** (Serum / Kilohearts / Guitar Rig 8) — dark radial gradient backgrounds, frosted-glass surfaces with `backdrop-filter: blur`, subtle glow on accent states

Both are **the same language**, not two languages. Surface treatments, border hardness, and glow intensity flex between modes; layout, typography, and palette are shared.

### 2. Application structure

Three main UI sections:

- **Header** — logo; segmented icon tabs (Grid, Tuner, MIDI, System); preset + scene dropdowns with fused MIDI-assign buttons; theme toggle (sun/moon icon)
- **Main area** — the Grid with a floating Options Panel for the selected block
- **Footer** — CPU + IN + OUT meters, each with label / rail / readout

The current sidebar options-panel convention is replaced by a floating panel inside the main area (scope deferred to Phase 4). A separate Plugins tab was considered in the mockups but is NOT part of the app — plugin management lives in the System tab's Settings page.

### 3. Colour palette

Four brand tokens + semantic tokens + 10-step grey ramp. **Nothing else**. Ad-hoc colours are not permitted in the codebase going forward.

#### Brand

| Token | Role | Light hex | Dark hex | Notes |
|---|---|---|---|---|
| `--accent` | Primary — selection, active state, meter fill (healthy), focus rings, logo mark | `#c026d3` *(Orchid-600)* | `#d946ef` *(Orchid-500 with glow)* | "Saturated Orchid"; warm purple; best CVD resilience of the candidates tested |
| `--accent-text` | Accent-coloured text | `#a21caf` *(Orchid-700)* | `#f0abfc` *(Orchid-300)* | Ensures ≥ 6:1 contrast on mode background |
| `--secondary` | Preset/scene indicator, meter "hot" zone (pre-clip), dirty state | `#f59e0b` *(Amber-500)* | `#fbbf24` *(Amber-400 with glow)* | Different semantic to "active" so the two don't fight visually |

#### Semantic

| Token | Role | Hex | Notes |
|---|---|---|---|
| `--success` | Confirmation, safe state | `#10b981` *(Emerald-500)* | Modern, CVD-resilient, always paired with a ✓ icon |
| `--danger` | Error, destructive action, clip indicator | `#e11d48` *(Rose-600)* | Slight pink pull harmonises with Orchid; still unambiguously danger |

#### Neutrals

Full 10-step grey ramp shared across modes, with semantic aliases:

| Alias | Light value | Dark value |
|---|---|---|
| `--bg` | grey-50 (`#fafbfc`) | radial gradient over grey-0 (`#050510`) through `#0a0a1e` and `#1e1a3e` |
| `--surface` | grey-0 (`#ffffff`) | `rgba(255,255,255,0.03)` with `backdrop-filter: blur(20px)` |
| `--border` | grey-200 (`#e5e7eb`) | `rgba(255,255,255,0.25)` — inputs, controls (higher opacity for visibility) |
| `--divider` | grey-200 (`#e5e7eb`) | `rgba(255,255,255,0.1)` — chrome separators (subtler than `--border`) |
| `--text` | grey-900 (`#1c1e22`) | grey-900 dark (`#e8f1fc`) |
| `--text-muted` | grey-500 (`#6b7280`) | grey-500 dark (`#6b7e96`) |
| `--text-subtle` | grey-400 (`#9ca3af`) | grey-400 dark (`#9ba3b4`) |

Rule: inputs and other interactive bordered controls use `--border`. Chrome separators (header/footer dividers, panel edges) use `--divider`.

Full 10-step ramps for Orchid, Amber, and Grey are captured in the brainstorm mockups (see appendix).

### 4. Typography

**Typeface:** Switzer (variable, project standard — already loaded).

**Principle:** hierarchy is expressed through **weight, colour, and whitespace** — not size. If the design needs more than one heading size, the system has failed.

| Token | px | Weight | Usage |
|---|---|---|---|
| `--text-xs` | 13 | 500 | Small labels only: CPU/IN/OUT, FREQ/GAIN/Q, unit hints. **13px is the minimum size anywhere.** |
| `--text-base` | 15 | 400 | Default for everything: body, meter values, tab labels, preset name, scene letters |
| `--text-base-strong` | 15 | 600 | Emphasis: panel headings, active tab text, selected scene letter |
| `--text-display` | *reserved* | *reserved* | Measured-value readouts (Tuner primary reading, future LCD-style displays). Size locked when first use case is designed. |

**Other rules:**

- Weights used: 400, 500, 600, 700 (700 reserved for rare strong emphasis)
- Letter spacing: `0.01em` default; `0.08em` uppercase labels; `0` panel headings; `-0.01em` logo/display-size text
- `font-variant-numeric: tabular-nums` everywhere digits could re-align (meters, parameter values, time readouts)
- Line height: 1.4 for body, 1.2 for headings, 1.0 for display numerics

### 5. Dimension tokens

Shared sizing tokens — all interactive elements read from these. One change here updates every input / button / select in the app.

| Token | Value | Usage |
|---|---|---|
| `--radius` | 6px | border-radius on every input, button, select, dialog, tag |
| `--input-height` | 32px | height of every input, button, select trigger, InputGroup |
| `--input-height-sm` | 24px | small variant (badges, compact icon tags) |
| `--input-padding-x` | 0.6rem | horizontal padding for text-bearing inputs |
| `--border-container` | `none` light / `1px solid var(--color-border)` dark | optional outline for tinted containers (e.g. tab list) — visible only in dark mode |

### 6. Primitive components

Shipped in Phase 1 (`ui/src/components/common/`). All interactive elements in the app use these — no bespoke `<input>` / `<button>` styling.

| Component | Purpose | Key props |
|---|---|---|
| `Input` | Text input | `inGroup` strips border/radius + sets `flex: 1` for use inside InputGroup |
| `IconButton` | Bordered icon button | `icon` (ReactNode); `inGroup` strips border/radius |
| `Button` | Text button | `variant`: `default` (orchid hover) / `secondary` (amber hover) / `danger` (red hover); `active` prop for toggle states; `size`: `default` / `sm` |
| `InputGroup` + `InputGroupLabel` | Composable fused container | children can be Input/IconButton/Button with `inGroup`, or use `--trigger-border: none` + `--trigger-radius: 0` for Radix-styled triggers |
| `ToggleSwitch` | On/off toggle (separate interaction pattern) | — |

**Interaction pattern for bordered elements:**
- Hover → `border-color: var(--color-secondary)` (amber) + `background: color-mix(in srgb, var(--color-secondary) 8%, transparent)`
- Focus (inputs) → same amber border
- Active (selected state) → `border-color: var(--color-primary)` (orchid) + orchid tint

Dropdowns styled via Radix (PluginSelect `.trigger`, Select triggers) expose `--trigger-border` and `--trigger-radius` CSS variables so they can be fused into input groups without modifying their React markup.

### 7. Accessibility principles

- All accent tokens have been verified under simulated deuteranopia (~6% of men), protanopia (~2%), and tritanopia (< 0.01%). Warm Orchid retains its purple identity under CVD better than cool violets would.
- Semantic state colours (success / danger) always appear alongside an icon (✓ / ⚠), never colour alone.
- Contrast ratios meet or exceed WCAG 4.5:1 for body text on both light and dark backgrounds when using the designated `-text` variants.
- Minimum text size is 13px everywhere.

### 8. What is deliberately NOT decided in Phase 0

Deferred to later phases:

- **Grid / canvas mechanics** — React Flow adoption, snap-to-grid, pan/zoom, dynamic sizing (Phase 3)
- **Options panel interaction model** — floating vs docked, positioning, z-index layering, dismiss behaviour (Phase 4)
- **Motion / animation language** — hover transitions, focus reveals, glow pulsing (Phase 4 polish)
- **`--text-display` concrete value** — deferred until Tuner redesign

Delivered in Phase 1:
- Design tokens + theme switching (`ui/src/design/tokens.css`, `useThemeStore`, `useSyncTheme`)
- Primitive components (`Input`, `IconButton`, `Button`, `InputGroup`)
- Icon set: `react-icons` (Tabler `tb/*` + Lucide `lu/*`)

Delivered in Phase 2:
- Footer with CPU / IN / OUT meters
- Header with segmented icon tabs + preset/scene dropdowns + theme toggle
- Token sweep across all existing components (no hardcoded hex remaining in CSS modules)

## Implementation status (2026-04-17)

| Phase | Scope | Status |
|---|---|---|
| 0 · Design direction | This doc | Locked, living as tokens evolve |
| 1 · Design system | Tokens, theme switch, primitives | Shipped (PRs #35, #37) |
| 2 · Header + footer | Footer meters, icon tabs, preset/scene chrome, theme toggle | Shipped (PR #36 + in-flight) |
| 3 · Canvas migration | React Flow evaluation + grid/ConnectionLayer migration | Not started |
| 4 · Options panel + polish | Floating panel, dialog polish, motion | Not started |

## Reference mockups

The Phase 0 exploration produced mockups that persist under `.superpowers/brainstorm/21891-*/content/`. Key screens:

- `aesthetic-direction-v2.html` — four schools (A/B/C/D) with differentiated DNA
- `fullapp-mockup-all-four.html` — all four schools applied to the Stellarr layout
- `fresh-purples.html` / `accessible-purples.html` — purple exploration with CVD simulation
- `locked-palette-and-chrome.html` — locked-candidate palette applied to header + footer in both modes
- `semantic-colours.html` — green/red selection with CVD sims

These are non-authoritative references — the tokens in *this* doc are the truth.
