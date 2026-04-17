# Settings UI spec-cleanup design

**Status:** draft 2026-04-17
**Author:** brainstorm session (Claude + Rey)
**Scope:** Bring the Settings screen (Libraries + Discovered Plugins + Privacy + info panel) into full compliance with the Phase 0 design system. Third of three screen-level cleanup specs before Phase 3.

## Context

Tuner cleanup landed in PR #39, MIDI cleanup in PR #40. Settings is the last screen on the pre-Phase-3 cleanup list. After this, all four main screens (Grid, Tuner, MIDI, Settings) will use tokens + primitives consistently, unblocking the Phase 3 canvas migration without carrying design-system tech debt forward.

## Audit findings

### `ui/src/components/settings/Settings.tsx` + `Settings.module.css`

- Directory-remove control rendered as a literal "x" inside `Button variant="danger" size="sm"` — should be `IconButton size="sm"` + `TbX`, matching the MIDI row-remove pattern.
- `.dirRow`, `.pluginRow`, `.privacyRow` are individually bordered boxes — inconsistent with the borderless-list pattern established by MIDI cleanup.
- `.dirBadge` ("System" read-only indicator) uses `--color-secondary` (amber) — misuses the interactive-intent colour role for a static badge. Also sized at 1rem, no fixed height, inconsistent with the `Tag`-style chip visual introduced in the Tuner cleanup.
- Widespread `font-size: 1rem` (16px) — above `--text-base` (15px).
- `.privacyDescription` 0.85rem — below the 13px minimum.
- `.infoTitle` 1.2rem — branding display-size; acceptable per Phase 0's "--text-display deferred" decision.

## Design decisions

1. **Rows** — `.dirRow`, `.pluginRow` become borderless lists with 1px `--color-divider` separators (`border-top` on the container + `border-bottom` on each row). `.privacyRow` is a single-item section; rendered as a plain padded flex row with no divider — the `Section` border already separates it.
2. **Remove directory** — `IconButton size="sm"` + `TbX size={14}`, consistent with MIDI row-remove.
3. **"System" badge** — styled span sized like a non-interactive chip: `--input-height-sm` height, `--text-xs`, `--color-border` border, `--color-muted` text. Neutral — conveys "read-only" without competing with interactive state colours.
4. **Type scale sweep** — all `1rem` usages → `--text-base`; all supporting text → `--text-xs`. Plugin manufacturer, plugin format, privacy description, dir badge, info version all drop to 13px.
5. **`.infoTitle`** kept as 1.2rem bold orchid with text-shadow glow (branding; Phase 4 motion polish will revisit).
6. **`.infoVersion`** gains `font-variant-numeric: tabular-nums` (it's a version number).

## Files touched

| Path | Change |
|---|---|
| `ui/src/components/settings/Settings.tsx` | `IconButton` + `TbX` for remove; no structural changes beyond that |
| `ui/src/components/settings/Settings.module.css` | rewrite row treatments + full type-scale sweep |

No primitive changes required. Build on MIDI cleanup's `IconButton size="sm"`.

## Non-goals

- `.infoPanel` redesign (branding panel; Phase 4 polish).
- `--text-display` token lock.
- Settings navigation redesign (currently single scroll).
- Phase 3 canvas migration.

## Testing

- `cd ui && npm run test` — existing 39 tests pass.
- `npx tsc --noEmit` clean.
- `npm run build` clean.
- Manual: Add Directory button, Scan Now button, remove a non-default directory via × icon, toggle crash reporting. Verify badge visual for "System" directories. Verify light + dark themes.

## Branch + PR

- Branch: `chore/ui-settings-spec-cleanup`
- Merge: squash.
