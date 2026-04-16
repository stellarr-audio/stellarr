# Phase 0 · Design Direction

**Status:** approved 2026-04-15
**Author:** brainstorm session (Claude + Rey)
**Scope:** whole-app visual redesign, Phase 0 only — establishes the visual north star. Does not specify implementation.

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

- **Header** — logo; icon tabs (Grid, Tuner, MIDI, Plugins, Settings); preset selector + scene buttons (A/B/C/D)
- **Main area** — the Grid with a floating Options Panel for the selected block
- **Footer** — CPU meter + Input meter + Output meter, each with label / rail / readout

The current sidebar options-panel convention is replaced by a floating panel inside the main area (scope deferred to Phase 4).

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
| `--border` | grey-200 (`#e5e7eb`) | `rgba(255,255,255,0.06)` |
| `--text` | grey-900 (`#1c1e22`) | grey-900 dark (`#e8f1fc`) |
| `--text-muted` | grey-500 (`#6b7280`) | grey-500 dark (`#6b7e96`) |
| `--text-subtle` | grey-400 (`#9ca3af`) | grey-400 dark (`#9ba3b4`) |

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

### 5. Accessibility principles

- All accent tokens have been verified under simulated deuteranopia (~6% of men), protanopia (~2%), and tritanopia (< 0.01%). Warm Orchid retains its purple identity under CVD better than cool violets would.
- Semantic state colours (success / danger) always appear alongside an icon (✓ / ⚠), never colour alone.
- Contrast ratios meet or exceed WCAG 4.5:1 for body text on both light and dark backgrounds when using the designated `-text` variants.
- Minimum text size is 13px everywhere.

### 6. What is deliberately NOT decided in Phase 0

The following are scoped out of this spec and will be decided in their respective phases:

- **Grid / canvas mechanics** — whether we adopt React Flow, snap-to-grid behaviour, pan/zoom, dynamic sizing (Phase 3)
- **Options panel interaction model** — floating vs docked, positioning, z-index layering, dismiss behaviour (Phase 4)
- **Component primitives** — button, input, toggle, slider, dropdown etc. as actual code (Phase 1)
- **Icon set** — choosing a library or commissioning custom (Phase 1)
- **Motion / animation language** — hover transitions, focus reveals, glow pulsing (Phase 1)
- **`--text-display` concrete value** — deferred until Tuner redesign

## Reference mockups

The Phase 0 exploration produced mockups that persist under `.superpowers/brainstorm/21891-*/content/`. Key screens:

- `aesthetic-direction-v2.html` — four schools (A/B/C/D) with differentiated DNA
- `fullapp-mockup-all-four.html` — all four schools applied to the Stellarr layout
- `fresh-purples.html` / `accessible-purples.html` — purple exploration with CVD simulation
- `locked-palette-and-chrome.html` — locked-candidate palette applied to header + footer in both modes
- `semantic-colours.html` — green/red selection with CVD sims

These are non-authoritative references — the tokens in *this* doc are the truth.

## Next steps

Once this spec is approved, move to **Phase 1 planning**: translate these tokens into `ui/src/design/` CSS variables, set up the theme-switch mechanism, and identify which primitive components exist today that need migration.
