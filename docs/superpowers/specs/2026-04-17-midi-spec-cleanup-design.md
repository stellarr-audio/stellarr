# MIDI UI spec-cleanup design

**Status:** draft 2026-04-17
**Author:** brainstorm session (Claude + Rey)
**Scope:** Bring the MIDI screen (mappings list + monitor/sender side panel) into full compliance with the Phase 0 design system. Extend `IconButton` with a `size="sm"` variant. Second of three screen-level cleanup specs before Phase 3.

## Context

Tuner cleanup landed in PR #39 (`2026-04-17-tuner-spec-cleanup-design.md`). This spec covers MIDI. Settings will follow as a third spec.

## Audit findings

### `ui/src/components/midi/MidiPage.tsx` + `MidiPage.module.css`
- Raw `<button>` for "Clear All" and row remove.
- `Cross2Icon` from `@radix-ui/react-icons` — design spec uses Tabler/Lucide only.
- `.title` 1.2rem, `.tableHeader` 0.8rem (below 13px min), `.row` 0.95rem, `.emptyState` 1rem — all off-scale.
- `.row:hover` uses background-only (`var(--color-border)` — a border token used as bg); spec hover pattern is amber border + 8% tint.
- `.row` has no `border-radius`; 1px full-box border per row adds visual weight.
- `.target` rendered in amber (`--color-secondary`) — misuses "interactive intent" colour role.

### `ui/src/components/midi/MidiMonitor.tsx` + `MidiMonitor.module.css`
- 3 raw `<input>` (CC#, Ch, Value) and 2 raw `<button>` (Clear, Send) bypass primitives.
- `.sendBtn` hardcoded `#ffffff` fg with `--color-primary` bg — bespoke filled-primary not in the primitive vocabulary.
- `.sectionTitle` 1rem muted uppercase — inconsistent with the Tuner-established convention (panel header muted; grouping headers orchid).
- `.fieldLabel` 0.7rem (below min); `.fieldInput` 0.85rem bespoke; `.log` 0.75rem.
- `.logType` amber — decorative misuse.

## Design decisions

1. **MidiPage rows** — borderless list with 1px `--color-divider` dividers between rows; rounded-top-bottom eliminated (reads as data table, not card stack). Hover: 8% amber bg tint (no border; border would re-introduce the box look).
2. **"Clear All"** — `Button variant="danger" size="sm"` (destructive global action).
3. **Row remove** — `IconButton size="sm"` (24px), bordered per primitive default. Swap `Cross2Icon` for `TbX`.
4. **CC Sender layout** — three stacked `InputGroup`s with labels (`CC#`, `Ch`, `Val`), then full-width `Button` "Send" beneath. Keeps the existing labelled mental model; all three inputs are now 32px primitive `Input`.
5. **Send button** — default bordered `Button` (no new `variant="primary"` filled style). Section header + context sell it as the primary action; keeps the primitive vocabulary minimal.
6. **Section headers** — "Monitor" / "Send CC" → `--color-primary` (orchid) per Tuner convention. Panel header "MIDI" added at top for consistency with TunerPanel structure (muted uppercase).
7. **Log monospace body** → system monospace stack, `--text-xs`. `logType` neutral text (bold); `logData` muted + tabular-nums.

## Primitive change

`IconButton` gains a `size?: 'default' | 'sm'` prop. `sm` = `--input-height-sm` (24px), square-ish, smaller horizontal padding. Mirrors `Button`'s existing `size` prop. No other behavioural change.

## Files touched

| Path | Change |
|---|---|
| `ui/src/components/common/IconButton.tsx` | add `size` prop |
| `ui/src/components/common/IconButton.module.css` | add `.sm` rule |
| `ui/src/components/midi/MidiPage.tsx` | primitives; `TbX` icon |
| `ui/src/components/midi/MidiPage.module.css` | rewrite (borderless list, type scale, hover pattern) |
| `ui/src/components/midi/MidiMonitor.tsx` | primitives; structural panel header + dividers to match TunerPanel |
| `ui/src/components/midi/MidiMonitor.module.css` | rewrite (type scale, orchid section titles, primitive `Input`/`Button` only) |

## Non-goals

- Monitor/Sender split as tabs (still stacked; Tablist not justified with only two stacked sections).
- MidiAssignDialog redesign (used from multiple surfaces; separate pass).
- Settings screen cleanup (separate spec).
- Phase 3 canvas migration.

## Testing

- `cd ui && npm run test` — existing 39 tests pass.
- `npx tsc --noEmit` clean.
- `npm run build` clean.
- Manual: open MIDI tab, verify mappings list renders / hover / row click / delete; Clear All is now danger styled; CC sender sends; Clear clears log; light + dark themes both readable.

## Branch + PR

- Branch: `chore/ui-midi-spec-cleanup`
- Merge: squash.
