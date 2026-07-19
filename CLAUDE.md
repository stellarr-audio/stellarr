# Stellarr — Claude Code Guidelines

## Project Overview

Open-source guitar signal processing standalone app. JUCE (C++) audio engine with React + TypeScript UI connected via JUCE WebView bridge.

## Development Approach

## UI / UX philosophy

**Always think first about what is best for the END USER.** Before locking any UI / UX decision, audit it against this directive — not after a bug report. Convenience-driven shortcuts that worsen the user's experience are rejected; find the user-respecting alternative.

- **Accessibility before convenience.** Readable text, sensible contrast, enforceable floor sizes that no setting can break. WCAG-aligned defaults (≥11px for body text, sufficient colour contrast). When proportional scaling can shrink elements below those floors, use `max(<floor>, …)` clamps so the floor cannot be defeated.
- **Discoverability before density.** Place affordances where users naturally look — anchor controls to the workspace they affect, not to wherever was easiest to mount in code. A toolbar that controls the Grid view sits in the Grid view, not on a tab.
- **Predictable behaviour and persistence.** Settings, zoom levels, panel positions, and theme choices persist across launches unless deliberately ephemeral (dev-only state). When something is intentionally ephemeral, document why.
- **Default states reflect production usage.** Dev / debug / experimental surfaces hide by default. The first-launch experience is what a non-technical end-user expects to see.
- **Surface intent in failure modes.** Disabled buttons say why they are disabled (tooltip / title). Empty states explain what to do. Errors point at recovery, not at the engine.

When proposing or implementing a UI change, ask "is this best for the end user?" and reject the alternative until you can answer yes.

## Architecture philosophy

Default to the long-term, architecturally correct approach. Do not offer "quick fix vs proper fix" tradeoffs unless explicitly asked. Assume full scope of change is acceptable. Correctness, maintainability, and performance take priority over minimal diff size.

- Follow existing code patterns, naming conventions, and project structure unless the task
  involves improving them
- Ask for clarification when requirements are ambiguous rather than assuming
- For significant changes: explain the approach and wait for approval before implementing
- Highlight breaking changes and cross-cutting dependencies before starting
- Respect existing test coverage; extend it where changes warrant
- **Default execution mode is subagent-driven** (`superpowers:subagent-driven-development`):
  once a spec and plan are approved, dispatch a fresh subagent per task with review between
  tasks. Don't ask which mode to use unless opted out for a specific task

## Communication Standards

- Keep responses focused and actionable
- No emojis in any output, logs, or generated content
- No auto-generated documentation files unless requested
- No conversation summaries after each successful code edit
- Use New Zealand English spelling and conventions

## Review Standards

When asked to review code (or variations like "pls review", "review and commit", "review deeply"), always check for:

- DRY violations and duplicated logic that should be extracted
- Dead code, unused imports, unreachable branches
- Readability: unclear variable names, missing comments on non-obvious logic
- Consistency with existing codebase patterns and conventions
- Error handling at system boundaries (user input, external APIs, plugin loading)
- Best practices for the relevant language (C++ and TypeScript)
- Build verification (`make dev-ui` or `npx tsc --noEmit` for UI, `make debug` for engine)
- Run all tests before committing (`make test`)

## Documentation

- User manual lives in `docs/manual/` as Markdown files with Starlight frontmatter (`title`, `description`, `sidebar:{order}`). Dev test cases live in `docs/testing/` with the same frontmatter shape. Both are rendered by the Astro site in `web/` and served at `stellarr.org/docs/*`.
- When adding or significantly changing a user-facing feature, update the relevant manual page
- Match the existing tone: concise, second-person, no jargon without explanation
- Use New Zealand English spelling consistent with the rest of the project
- Do not create new manual pages without being asked — prefer extending existing ones
- Reference other pages with absolute URL paths (e.g., `[MIDI](/docs/midi/)`) — Starlight resolves them.
- **Do not commit design specs, brainstorming specs, or implementation plans to the repo.** Durable project truth lives in `docs/manual/`, `CLAUDE.md`, PR descriptions, and GitHub release notes. If a decision needs to outlive the session, promote it into one of those surfaces.
- **Working specs live under `.superpowers/brainstorm/<session-id>/`** (gitignored). The superpowers visual-brainstorming server writes its HTML mockups there; write per-feature spec markdown (e.g. `spec.md`) next to them. Survives restarts, invisible to git, discoverable by future sessions — list `.superpowers/brainstorm/` to pick up prior work. Default spec path: `.superpowers/brainstorm/<session-id>/spec.md` rather than `docs/superpowers/specs/` (which is off-limits).

## Git Workflow

- **Never commit directly to main** — main is protected
- Branch from main, push branch, create PR, merge via GitHub
- CI runs automatically on PRs and must pass before merge

### Before writing any code

1. Check the current branch (`git branch --show-current`)
2. If on `main` or an unrelated branch: stash any uncommitted work, switch to `main`, pull latest, and create a new branch
3. One branch per task — never mix unrelated changes on the same branch
4. If already on the correct branch for the current task, continue on it

### Commit messages

Follow [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/):

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

- **Types:** `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`
- Description in imperative mood, present tense (e.g. `fix: resolve crash on load`)
- First line under 72 characters
- Reference GitHub issue numbers where applicable (e.g. `fix: resolve scan freeze (#12)`)
- Use `feat!:` or a `BREAKING CHANGE:` footer for breaking changes

### Committing and PRs

> **HIGH PRIORITY — non-negotiable gates.**
>
> 1. **Verify in the running app before committing.** Never commit or push until the user has confirmed the change actually works by running it. Type-check / tests / build passing is necessary but NOT sufficient. For UI or runtime-behaviour changes, state what to test and which command to run (`make run-ui`, `make open`, `make dev`) and wait for the user's sign-off before `git add` / `git commit`.
> 2. **Never open a PR without explicit user confirmation.** Even after verification and committing, do not run `gh pr create` (or push a branch with the intent of opening a PR) until the user explicitly says so. Commits on a branch are fine; opening a PR is a separate decision the user makes.

1. Stage only the files relevant to the task.
2. **Local code review before commit.** Run `codex review --uncommitted` for non-trivial changes and address findings inline.
   - **What it catches:** contrast / design-system / specificity issues, drift between docs and code, missed edge cases — before the cloud Codex bot sees them, saving review round-trips.
   - **Run for:** engine logic, UI behaviour or styling, multi-file refactors, and docs that describe code behaviour, command names, file paths, or configuration (e.g. user manual pages, testing checklists).
   - **Skip for:** typo / grammar / wording-only edits, dependency bumps, version chores, trivial style tweaks.
   - **Model:** Codex picks its current default. Pin explicitly with `-c model=<name>` only when there's a reason.
3. Commit with a Conventional Commits message only after the user verifies the change works.
4. Wait for the user to explicitly request a PR before pushing / opening one.
5. Once a PR is open, wait for CI to pass before requesting merge.
6. After merge, delete the remote branch and switch back to `main` locally.

### Branch naming

| Type | Pattern | Example |
|------|---------|---------|
| Feature | `feature/<short-name>` | `feature/undo-redo` |
| Bug fix | `fix/<short-name>` | `fix/scan-freeze` |
| Hotfix | `hotfix/<short-name>` | `hotfix/crash-on-load` |
| Documentation | `doc/<short-name>` | `doc/update-midi-manual` |
| Chore | `chore/<short-name>` | `chore/bump-actions` |

### Merge strategy

- **Squash merge** for features and fixes (clean single commit on main)
- **Regular merge** for large features where individual commits tell a story
- Delete branch after merge
- Tag from main for releases (`git tag v1.x.x`)

### Versioning + releases

- SemVer: MINOR bump for new user-facing behaviour / design-system additions; PATCH for polish, fixes, refactors with no functional change.
- **Before tagging a release**, bump `ui/package.json`'s `"version"` field in a `chore: bump to vX.Y.Z` commit. Run `npm install` in `ui/` to sync `ui/package-lock.json`. That's it — CMake reads the version from `ui/package.json` at configure time (single source of truth for UI, engine, Info.plist, and window title), Vite injects `__APP_VERSION__` at build time.
- Merge the bump PR, then tag from main (`git tag vX.Y.Z`) and `gh release create vX.Y.Z` with the changelog in the release body. The GitHub release body is the authoritative changelog — no `CHANGELOG.md` in-repo.
- The release workflow signs the DMG with the Sparkle EdDSA private key (GitHub Actions secret `SPARKLE_PRIVATE_KEY_PROD`) and opens a bot PR against `main` that appends the new release to `web/public/appcast.xml`. Merging that PR publishes the update to existing installs via `stellarr.org/appcast.xml`. No manual appcast edits.
- Rotate Sparkle signing keys with `make regen-sparkle-keys-prod` / `make regen-sparkle-keys-dev`. Prod rotation invalidates update trust for every already-installed build — only rotate on a deliberate key-compromise response.

## Build Commands

- `make dev` — build UI + engine (no tests), clears WebView cache
- `make debug` — build UI + engine with tests
- `make release` — optimised build
- `make run` / `make run-debug` / `make run-release` — build and launch
- `make run-ui` — UI-only rebuild + relaunch existing engine binary (fast iteration for CSS/React)
- `make test` — build with tests and run them
- Always run `make dev-ui` or `npx tsc --noEmit` to catch TypeScript errors, not just `make`

### Prerequisites

macOS Apple Silicon, CMake 3.24+, Xcode CLI tools, Node.js 18+, npm. See `docs/CONTRIBUTING.md` for full setup.

## Architecture Notes

- Engine (C++): JUCE AudioProcessorGraph for audio routing
- UI (React/TS): Zustand store, Radix UI components, CSS Modules
- Bridge: UI `sendEvent` -> C++ `handleEvent` -> `emitToJs` -> Zustand store
- Bridge handlers are split by domain: `engine/bridge/` contains PresetHandler, GraphHandler, SceneHandler, MidiHandler, ParamHandler (all compiled as part of StellarrBridge)
- Tests: C++ test executables with custom harness (each has its own `main()`), gated behind `BUILD_TESTING` CMake flag. New audio processing features should have a corresponding test in `engine/test/`.
- Manual tests: `docs/testing/` contains per-area test case files (TC-XX-NNN format). When adding automated tests, also add corresponding manual test cases for anything that needs real plugins, hardware, or human judgement.
- Settings persistence: JUCE `ApplicationProperties` / `PropertiesFile` stored in `~/Library/Application Support/Stellarr/`
- Declarations in headers, implementations in .cpp files
- Licence: AGPLv3 (required by JUCE dependency) — all contributions must be compatible

### Audio thread safety

- Never modify the AudioProcessorGraph from the audio thread
- Batch graph mutations with `UpdateKind::none` and call `rebuildGraph()` once at the end — never trigger N intermediate rebuilds
- Use `suspendProcessing(true)` on the top-level processor to synchronise with the audio thread (it acquires the `callbackLock`)
- Pre-create plugin instances (load binary, `prepareToPlay`) before suspending the graph to minimise the audio gap
- Use `std::atomic` for data shared between audio and message threads; use `SpinLock::ScopedTryLockType` (non-blocking) on the audio thread
- Individual block `suspendProcessing` has no effect at graph level — the graph does not check `isSuspended()` on sub-nodes

### Key paths

| Area | Path |
|------|------|
| Engine entry | `engine/StellarrStandaloneApp.cpp` |
| UI entry | `ui/src/main.tsx` |
| Bridge (C++) | `engine/StellarrBridge.cpp`, `engine/bridge/` |
| Bridge (TS) | `ui/src/bridge/index.ts` |
| Zustand store | `ui/src/store/index.ts` |
| C++ tests | `engine/test/` |
| User manual | `docs/manual/` |
| Dev test cases | `docs/testing/` |
| Website (Astro + Starlight) | `web/` |

## Design system (UI)

**Source of truth:** this CLAUDE.md section plus the actual code in `ui/src/design/tokens.css` and `ui/src/components/common/`. Keep those in sync with reality.

### Golden rules

- **No ad-hoc hex in CSS modules.** Pick an existing token. If none fits, propose a new token in `tokens.css` — never hardcode.
- **No bespoke `<input>` / `<button>` styling.** Use the primitive components (`Input`, `IconButton`, `Button`, `InputGroup`, `ToggleSwitch`). Extending? Pass `className` for layout, not for colours/sizing.
- **Primary (orchid) = active/selected.** Secondary (amber) = hover/interactive intent. Never flip these roles.
- **Weight for hierarchy, not size.** Two text sizes only (13/15). If you need a third, the design failed — push back.
- **Width of interactive elements uses `var(--input-height)`**: 32px. Never hardcode pixel heights on inputs/buttons/selects.

### Key files

| Role | Path |
|---|---|
| Tokens (source of truth) | `ui/src/design/tokens.css` |
| Legacy alias layer (`--color-*`) | `ui/src/styles/variables.css` |
| Font declarations (`--font-sans`, `--font-mono`) | `ui/src/assets/fonts/fonts.css` |
| Primitive components | `ui/src/components/common/{Input,IconButton,Button,InputGroup,ToggleSwitch,Numeric}.tsx` |
| Theme store | `ui/src/store/theme.ts` |
| Theme sync hook | `ui/src/hooks/useSyncTheme.ts` |

### Palette (semantic tokens)

| Token | Role | Light | Dark |
|---|---|---|---|
| `--color-primary` (`--accent`) | Selected, active, brand identity | `#c026d3` Orchid-600 | `#d946ef` Orchid-500 |
| `--color-secondary` (`--secondary`) | Preset/scene indicator, hover hint, warning | `#f59e0b` Amber-500 | `#fbbf24` Amber-400 |
| `--color-green` (`--success`) | Confirmation (always + ✓ icon) | `#10b981` Emerald-500 | same |
| `--color-danger` (`--danger`) | Error, destructive, clip (always + ⚠ icon) | `#e11d48` Rose-600 | same |
| `--midi` / `--midi-text` | MIDI-assigned indicators (preset/scene tag, badge, link button) — distinct from accent and hover | `#0ea5e9` Azure-500 / `#0369a1` Azure-700 | `#38bdf8` Azure-400 / same |
| `--color-border` | Interactive control borders (inputs, buttons, selects) | `#e5e7eb` | `rgba(255,255,255,0.25)` |
| `--color-divider` | Chrome separators (header/footer/panel edges) | `#e5e7eb` | `rgba(255,255,255,0.1)` |
| `--color-bg` | Page background | grey-50 | radial gradient navy |
| `--color-surface` | Cards, elevated panels | white | `rgba(255,255,255,0.03)` + blur |
| `--color-text` / `--color-muted` / `--text-subtle` | Text scale | grey-900 / 500 / 400 | grey-dark-900 / 500 / 400 |

### Typography

Two typefaces, both SIL OFL 1.1, self-hosted as variable woff2. `@font-face` declarations live in `ui/src/assets/fonts/fonts.css`; the website mirrors them via `web/src/styles/fonts.css`.

- **Space Grotesk** (variable, weights 300–700) — everything that reads as text: labels, headings, body copy, all chrome surfaces. Exposed as the `--font-sans` token; the app root sets `font-family: var(--font-sans)`.
- **JetBrains Mono** — every numeric value and machine identifier: dB / Hz / cents / LUFS readouts, parameter values, MIDI labels (CC/PC), sample-buffer counts, version strings, plugin-format tags. Exposed as `--font-mono`. Applied via the **`<Numeric>` primitive** (`ui/src/components/common/Numeric.tsx`), which wraps numeric/identifier text and sets `font-family: var(--font-mono); font-variant-numeric: tabular-nums slashed-zero;` — never restyle colours/sizing on it, it inherits from context. Numeric *form fields* use the `mono` prop on the `Input` primitive instead (since `<Numeric>` cannot wrap an `<input>`). Do not hand-roll `font-variant-numeric: tabular-nums` on individual CSS rules — use `<Numeric>`.
- Slashed zero: `<Numeric>` applies `slashed-zero` automatically, distinguishing 0 from O at a glance — standard pro-audio convention.
- Chrome scale (panels, settings, dialogs, header, footer): `--text-xs` (13px, weight 500) · `--text-base` (15px, weight 400) · `--text-base-strong-weight` (600) · `--text-display` (reserved). Minimum 13px anywhere in chrome.
- **Weight for hierarchy, not size** — two text sizes only (13 / 15). If a third is needed, the design has failed; push back.

### Grid block scale + accessibility floors

The Grid is a viz surface — block-internal typography scales proportionally with the active cell zoom and is **not** bound to the chrome 13/15 scale. Set the `--block-scale` CSS variable on the Grid root from `useGridLayout().blockScale` (= `cellSize / 88`, M baseline). Block CSS uses:

```css
font-size: max(<floor>, calc(<base> * var(--block-scale, 1)));
```

Floors enforce WCAG-aligned readability regardless of zoom — they cannot be defeated by zooming out:

- `.blockType` (abbreviation tag like "IN" / "OUT" / "PLG"): floor `16px`
- `.pluginName` (block plugin label): floor `11px`
- Icons inside blocks: floor `14px` (apply via `Math.max(14, Math.round(<base> * blockScale))` in JS)

Chrome typography (panels, settings, dialogs, header) is NOT scaled by zoom — only the Grid viz surface scales. When adding new block-internal typography, default to `var(--block-scale)` consumption with a sensible floor.

### Dimension tokens

| Token | Value | Use |
|---|---|---|
| `--radius` | 0 | every interactive bordered element (sharp-edge direction) |
| `--input-height` | 32px | default height for inputs/buttons/selects/InputGroup |
| `--input-height-sm` | 24px | compact variant (badges, tags) |
| `--input-padding-x` | 0.6rem | horizontal padding for text inputs/buttons |
| `--border-container` | light: `none` / dark: `1px solid var(--color-border)` | optional outline for tinted containers (e.g. tab list) |

### Interaction patterns

- **Hover on bordered controls:** `border-color: var(--color-secondary)` + `background: color-mix(in srgb, var(--color-secondary) 8%, transparent)`. Transition `var(--transition-snap)` (180ms `cubic-bezier(0.2, 0, 0, 1)` — sharp-out, lands gently; replaces the older 150ms `ease`).
- **Focus on text inputs:** `border-color: var(--color-secondary)` (same colour as hover); `outline: none`. Rationale — **focus is a hover-in-place**: the user expects visual continuity, not a separate ring colour. Don't introduce a blue focus ring; **blue is reserved exclusively for azure MIDI indicators** (`--midi` / `--midi-text`) so colour-vision-deficient users can still distinguish "MIDI-assigned" from "focused" at a glance.
- **Active/selected:** `color: var(--color-primary)` + orchid tint background. Never blue/grey.
- **Tactile press:** Button / IconButton / Tag / Tablist tab apply `:active:not(:disabled) { transform: translateY(0.5px); }`. Press is instant (no transition on transform); the rest of the snap easing handles colour/border changes.
- **Section-title convention (Options panel):** orchid for grouping headers (Parameters, States). Neutral `var(--color-text)` for input labels (Plugin, Test Tone, Level, Target Loudness).
- **Slider design spec** (canonical):
  - Track: 4px tall, background `color-mix(in srgb, var(--color-muted) 25%, transparent)`. Sharp edges (`--radius: 0`).
  - Active fill: `var(--color-secondary)` (amber). Direction: from min toward the current value by default; flip to the right of the thumb (current → max) when the active range *is* the right-of-thumb region (e.g. binary "ON" trigger).
  - Thumb: square 16×16, `var(--color-secondary)` (amber) background, **darker-amber outline** `border: 2px solid var(--color-secondary-outline); box-sizing: border-box;` (`--secondary-outline` = `--amber-700` light / `--amber-600` dark). Sharp edges. Outline reads against the amber fill at any zoom; no surface-coloured halo, no layout shift on focus.
  - Focus-visible: **recolour the existing border** to `var(--color-primary)` (orchid) — `transition: border-color var(--transition-snap);` makes the colour swap feel continuous with the rest of the hover system. No extra outer ring.
  - Tick row (optional): 1px-wide × 4px-tall ticks in `var(--color-muted)` with 13px (`var(--text-xs)`) labels — wrap the value in `<Numeric>` for monospace + tabular-nums alignment.
  - Floating thumb value label (optional): 13px `var(--color-secondary)`, centred over the thumb — wrap in `<Numeric>` for monospace rendering. Clamp `left` to `[8%, 92%]` so it doesn't bleed past the track at extremes.
  - Reusable component: `ui/src/components/common/Slider.tsx` (Radix-backed). All sliders (Trigger, Options panel ParametersSection, SignalSection) consume this primitive — never roll a bespoke `<input type="range">` or duplicate styling.
- **Radix-controlled triggers** (Select/DropdownMenu) expose `--trigger-border` and `--trigger-radius` CSS variables — set on a parent to fuse a trigger into an `InputGroup` without modifying its markup.

### Floating panel titlebar (canonical pattern)

Used for the Options panel and any modal dialog (e.g. `MidiAssignDialog`). Every floating surface that has a title gets the same titlebar shape so the app reads as one design system.

```css
.titlebar {
  display: flex;
  align-items: center;
  padding: 0.4rem 0.6rem;
  background: var(--panel-titlebar);
  border-bottom: 1px solid var(--color-divider);
  gap: 0.35rem;
}
.titlebarText {
  font-size: var(--text-base);
  font-weight: var(--text-base-strong-weight);
  color: var(--color-text);
  letter-spacing: 0.08em;
  text-transform: uppercase;
  margin: 0;
}
```

- Titlebar sits at the top of the panel/dialog container; the body lives below it (with its own padding `0.75rem`).
- Container itself has no `padding` — the titlebar handles its own; the body handles its own.
- For modal dialogs, set `overflow: hidden` on the container so the titlebar's bottom border lines up with the side borders.
- Reference impls: `ui/src/components/options/OptionsPanel.module.css` (`.titlebar`, `.blockName`) and `ui/src/components/common/MidiAssignDialog.module.css` (`.titlebar`, `.titlebarText`).

### Icons

- Library: [`react-icons`](https://react-icons.github.io/react-icons/)
- Active sets: Tabler (`react-icons/tb`), Lucide (`react-icons/lu`), Ionicons 5 (`react-icons/io5`), Phosphor (`react-icons/pi`).
- Tabler is the default for new icons where multiple sets have an equivalent. The other sets are in use only because a specific glyph wasn't available in Tabler at sufficient quality (e.g. `PiTrafficSignal` for the Grid toolbar's MIDI test panel toggle, `LuSparkles` as the GridOverlay preset/scene separator, `IoCloseSharp` for hover-only block close affordance).
- Before introducing a fifth set, audit existing imports and exhaust the four current sets first.

### Theme

- Zustand store holds `theme: 'light' | 'dark' | 'system'`; persists under localStorage key `stellarr.theme`
- `useSyncTheme()` (called in `App.tsx`) writes `data-theme="light"|"dark"` on `<html>` and subscribes to `prefers-color-scheme` changes when `'system'`
- UI toggle in header flips between resolved light/dark (skips `'system'` to avoid invisible transitions)

### UI mockups + design previews

- **Preview-before-implement is mandatory for any new or modified visual UI.** When the user asks for a UI change (new component, restyled control, layout shift, colour swap), produce an HTML mockup first and wait for the user to pick / approve before touching `.tsx` / `.module.css`. Don't bundle "I'll show you the preview AND implement it" — split into two turns. Applies even to small tweaks; the user wants to see it before code lands.
- **Always render mockups in light AND dark mode side-by-side.** Stellarr ships both themes; a mockup that only shows one half is incomplete. Use a 2-column layout (light left, dark right) per variant, or two stacked sections — never a single theme.
- Use the real semantic tokens from `ui/src/design/tokens.css`. Don't hardcode hex values. Light theme palette is the `:root[data-theme="light"]` block; dark is `:root[data-theme="dark"]`. Copy the tokens needed (or load them via a `<style>` block in the HTML preview).
- Save mockup HTML under `.superpowers/brainstorm/<session-id>/content/` (gitignored, persistent across sessions, discoverable). Reference the absolute path back to the user so they can open it.

### Testing UI

- Runner: Vitest + JSDOM + `@testing-library/react`
- `cd ui && npm run test` — run all UI tests
- `cd ui && npm run test:watch` — watch mode
- Token tests use a `getVar()` helper (in `ui/src/design/__tests__/tokens.test.ts`) to resolve one level of `var()` indirection since JSDOM does not

### How to profile React renders

Runtime instrumentation is provided by [`react-scan`](https://github.com/aidenybai/react-scan), loaded dynamically inside an `import.meta.env.DEV` gate in `ui/src/main.tsx`. It is absent from production bundles — verify after any change to that gate with:

```bash
cd ui && npm run build
grep -rl "react-scan" dist/ && echo "FAIL: leaked into prod" || echo "OK: tree-shaken"
```

**Workflow.** Launch the dev app with `make run-ui`, trigger the scenario you are measuring, and read render counts off the react-scan overlay (and the browser console). When doing a perf audit, capture per-scenario counts into a markdown table in your session's `.superpowers/brainstorm/<session>/` working directory so any subsequent fix work can measure improvement against the baseline.

**Canonical render-sensitive scenarios.** Future audits should hit the same hotpaths so results stay comparable across releases:

1. Grid scroll + drag a block.
2. Grid cell-zoom change (S → M → L).
3. Options panel open / close on a block.
4. Slider drag (Mix / Balance / Level in Options panel).
5. State switch + scene recall.
6. MIDI monitor live event stream (20 Hz updates).
7. Tuner active (20 Hz strobe + Hz readout).
8. Preset switch (load preset → graph rebuild).
