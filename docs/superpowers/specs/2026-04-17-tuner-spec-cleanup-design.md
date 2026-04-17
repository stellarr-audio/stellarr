# Tuner UI spec-cleanup design

**Status:** draft 2026-04-17
**Author:** brainstorm session (Claude + Rey)
**Scope:** Bring the Tuner screen (main view + side panel) into full compliance with the Phase 0 design system. Extract two new primitives that unblock the same pattern elsewhere.

## Context

Phase 2 shipped the design system across the header, footer, Grid block chrome, Options panel, and shared primitives. The remaining screens (Tuner, MIDI, Settings) still carry pre-Phase-0 styling. An audit on 2026-04-17 surfaced concrete deviations.

This spec covers the **Tuner screen only**. MIDI and Settings cleanup will follow in separate specs once this lands.

Doing this before Phase 3 (canvas migration) keeps the design-system contract consistent across the whole app and avoids carrying tech debt into the canvas work.

## Audit findings (Tuner)

### `ui/src/components/tuner/Tuner.tsx` + `Tuner.module.css`
- Hardcoded `#ffaa00` on line 17 — should be `var(--color-secondary)` (via `colors.secondary`).
- `.note` (12rem), `.octave` (4rem), `.frequency` (1.8rem) — display-size numerics sit outside the locked type scale. `--text-display` is reserved but unset.
- `.labels` (cents ticks `-50`…`+50`) render digits but lack `font-variant-numeric: tabular-nums`. Also `font-size: 1rem` (16px) — above `--text-base`. `.frequency` already has `tabular-nums`.
- `.track` height 6px hardcoded (minor; single use).

### `ui/src/components/tuner/TunerPanel.tsx` + `TunerPanel.module.css`
- `.title` font-size 1rem (16px) — exceeds `--text-base` (15px).
- `.sectionTitle` font-size 1rem — same.
- Mode + preset buttons use `size="sm"` (24px), mismatching the `Input` (32px) inside the same panel. Row rhythm is uneven.
- Mode selector is a pair of `Button`s with `active` — a segmented control masquerading as two buttons.
- Preset row is also `Button`s with `active` — but semantically these are **discrete selectable values**, not actions.

### `ui/src/components/tuner/StrobeBand.tsx` + `StrobeBand.module.css`
- Hardcoded `#ffaa00` on line 12 — should be `var(--color-secondary)`.
- Band height 40px / center-mark 48px hardcoded (single-use; acceptable but noted).

## Design

### 1. New primitive: `Tablist`

A segmented pill group with the same visual language as the header tab list. Used anywhere the app needs "pick one of N mutually exclusive views or modes" inside a bounded container.

**Location:** `ui/src/components/common/Tablist.tsx` + `Tablist.module.css`

**Props:**

```ts
interface TablistItem {
  id: string;
  label: string;
  icon?: ReactNode;
}

interface TablistProps {
  value: string;
  onChange: (id: string) => void;
  items: TablistItem[];
  className?: string;
}
```

No `size` prop. Intrinsic height is driven by padding, matching the current header tablist. A second size variant can be added when a consumer needs it (YAGNI).

**Visual contract (extracted verbatim from `App.module.css`):**
- Outer: tinted bg (`color-mix(--color-muted 10%, transparent)`), `var(--border-container)`, `border-radius: 8px`, 3px inner padding, 2px gap.
- Item padding: `0.45rem 0.65rem` (same as current header tabs — no explicit height).
- Item idle: transparent, `color: var(--color-muted)`.
- Item hover: `color: var(--color-text)`.
- Item active: `background: var(--color-surface)`, `color: var(--color-primary)`, soft shadow.
- Transitions: 0.15s ease (colour, background, box-shadow).

**Implementation:** the primitive is a plain `<div role="tablist">` with `<button role="tab">` children. It does not depend on Radix — the header currently uses Radix `Tabs.Root` / `Tabs.List` / `Tabs.Trigger` / `Tabs.Content`, but only the triggers needed Radix behaviour (ARIA + keyboard nav). Radix is dropped from `App.tsx` entirely in this migration; the Tablist primitive handles ARIA (`role="tablist"`, `role="tab"`, `aria-selected`) and arrow-key navigation between tabs. `Tabs.Content` is replaced by conditional rendering based on `activeTab` state, which is how every other panel in the app is rendered anyway.

**Consumers migrated in this PR:**
- Header tabs in `App.tsx` — replace Radix `Tabs` entirely with `<Tablist>` + conditional rendering.
- Tuner Mode in `TunerPanel.tsx` — `<Tablist items={[{id:'needle',label:'Needle'},{id:'strobe',label:'Strobe'}]} />`.

**Out of scope:** MIDI sub-views, Options panel tabs. Primitive exists; adopt when they get refactored.

### 2. New primitive: `Tag`

A chip-style selectable value. Semantic role: "discrete quick-select alternative to typing". Distinct from `Button` (actions) and `ToggleSwitch` (on/off).

**Location:** `ui/src/components/common/Tag.tsx` + `Tag.module.css`

**Props:**

```ts
interface TagProps {
  active?: boolean;
  onClick?: () => void;
  children: ReactNode;
  className?: string;
}
```

**Visual contract:**
- Height: `var(--input-height-sm)` (24px).
- Padding: `0 0.6rem`.
- Radius: `var(--radius)`.
- Idle: border `1px solid var(--color-border)`, bg `transparent`, text `var(--color-text)`.
- Hover: border `var(--color-secondary)`, bg `color-mix(--color-secondary 8%, transparent)`.
- Active: border `var(--color-primary)`, bg `color-mix(--color-primary 10%, transparent)`, text `var(--color-primary)`.
- Font: `var(--text-xs)` weight 500, `font-variant-numeric: tabular-nums` (presets are numeric).
- Transition: 0.15s ease.

**Consumer migrated in this PR:**
- Tuner frequency presets (`432`, `440`, `442`, `444`) → `<Tag active={referencePitch === hz} onClick={…}>{hz}</Tag>`.

**Out of scope:** scene chips, block-type badges, MIDI channel chips, etc. Primitive exists; adopt as those surfaces come up.

### 3. Tuner.tsx fixes

- Replace `'#ffaa00'` (line 17) with `colors.secondary`.
- Add `font-variant-numeric: tabular-nums` to `.labels` and to `.frequency`.
- Leave display sizes (`.note`, `.octave`, `.frequency`) untouched — `--text-display` lock deferred to formal Tuner redesign task. **Justification:** locking a value without a second use case risks cementing the wrong size; Phase 0 explicitly flagged this.

### 4. TunerPanel.tsx fixes

- `.title` font → `var(--text-base)` (15px), weight 600 via `--text-base-strong-weight`. Keep uppercase + letter-spacing.
- `.sectionTitle` font → same. Keep orchid colour (section-group heading convention from the spec).
- Remove `size="sm"` from all Tuner buttons — no raw `Button`s remain in TunerPanel after Mode → `Tablist` and presets → `Tag` migration. `A4 / Hz` InputGroup `Input` already at 32px.
- Drop `.modeButton` + `.presetButton` CSS (no longer needed after primitive migration).

### 5. StrobeBand.tsx fixes

- Replace `'#ffaa00'` (line 12) with `colors.secondary`.
- Leave hardcoded band/center heights. They are geometry constants for a single-use visualisation. Token only when a second consumer emerges.

## Files touched

| Path | Change |
|---|---|
| `ui/src/components/common/Tablist.tsx` | **new** primitive |
| `ui/src/components/common/Tablist.module.css` | **new** |
| `ui/src/components/common/Tag.tsx` | **new** primitive |
| `ui/src/components/common/Tag.module.css` | **new** |
| `ui/src/App.tsx` | migrate header tabs to `Tablist` |
| `ui/src/App.module.css` | remove `.tabList`, `.tab`, `.tab:hover`, `.tabActive`, `.tabActive:hover` |
| `ui/src/components/tuner/Tuner.tsx` | hex → token; tabular-nums |
| `ui/src/components/tuner/Tuner.module.css` | tabular-nums on `.labels` + `.frequency` |
| `ui/src/components/tuner/TunerPanel.tsx` | Mode → `Tablist`; presets → `Tag` |
| `ui/src/components/tuner/TunerPanel.module.css` | `.title` + `.sectionTitle` to `--text-base`; drop `.modeButton`/`.presetButton` |
| `ui/src/components/tuner/StrobeBand.tsx` | hex → token |

## Non-goals

- MIDI and Settings screen cleanup (separate specs).
- Phase 3 canvas migration.
- Locking `--text-display` token value.
- Motion / glow polish (Phase 4).
- New visual patterns beyond what the audit requires.

## Testing

- `cd ui && npm run test` — existing Vitest token/component tests must still pass.
- New primitives get unit tests:
  - `Tablist`: renders items; calls `onChange` on click; applies active visual + `aria-selected` to the matching id; arrow keys move selection.
  - `Tag`: renders children; calls `onClick`; applies active class; honours `--input-height-sm`.
- `npx tsc --noEmit` clean in `ui/`.
- Manual: header tabs still switch screens; Tuner Mode still toggles needle/strobe; preset clicks still update reference pitch; Input at A4 still 32px and fused in InputGroup.
- `make build-ui` clean.

## Branch + PR

- Branch: `chore/ui-tuner-spec-cleanup`
- Merge: squash.
- CI must pass before merge.

## Follow-ups (out of scope)

- MIDI page cleanup spec.
- Settings page cleanup spec.
- `--text-display` token lock (deferred until Tuner redesign).
- Adopt `Tag` for scene/block-type chips as those surfaces are revisited.
- Adopt `Tablist` in Options panel / MIDI sub-views as those surfaces are revisited.
