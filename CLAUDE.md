# Stellarr — Claude Code Guidelines

## Project Overview

Open-source guitar signal processing standalone app. JUCE (C++) audio engine with React + TypeScript UI connected via JUCE WebView bridge.

## Development Approach

- Follow existing code patterns, naming conventions, and project structure
- Ask for clarification when requirements are ambiguous rather than making assumptions
- Plan before implementing significant changes — explain approach and wait for approval
- Prioritise simple, maintainable solutions over complex ones
- Respect existing testing patterns and coverage levels
- Highlight any potential breaking changes or dependencies
- Focus on the immediate task rather than broad refactoring

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
- Build verification (`make build-ui` or `npx tsc --noEmit` for UI, `make debug` for engine)
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
2. Commit with a Conventional Commits message only after the user verifies the change works.
3. Wait for the user to explicitly request a PR before pushing / opening one.
4. Once a PR is open, wait for CI to pass before requesting merge.
5. After merge, delete the remote branch and switch back to `main` locally.

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
- Always run `make build-ui` or `npx tsc --noEmit` to catch TypeScript errors, not just `make`

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
| Primitive components | `ui/src/components/common/{Input,IconButton,Button,InputGroup,ToggleSwitch}.tsx` |
| Theme store | `ui/src/store/theme.ts` |
| Theme sync hook | `ui/src/hooks/useSyncTheme.ts` |

### Palette (semantic tokens)

| Token | Role | Light | Dark |
|---|---|---|---|
| `--color-primary` (`--accent`) | Selected, active, brand identity | `#c026d3` Orchid-600 | `#d946ef` Orchid-500 |
| `--color-secondary` (`--secondary`) | Preset/scene indicator, hover hint, warning | `#f59e0b` Amber-500 | `#fbbf24` Amber-400 |
| `--color-green` (`--success`) | Confirmation (always + ✓ icon) | `#10b981` Emerald-500 | same |
| `--color-danger` (`--danger`) | Error, destructive, clip (always + ⚠ icon) | `#e11d48` Rose-600 | same |
| `--color-border` | Interactive control borders (inputs, buttons, selects) | `#e5e7eb` | `rgba(255,255,255,0.25)` |
| `--color-divider` | Chrome separators (header/footer/panel edges) | `#e5e7eb` | `rgba(255,255,255,0.1)` |
| `--color-bg` | Page background | grey-50 | radial gradient navy |
| `--color-surface` | Cards, elevated panels | white | `rgba(255,255,255,0.03)` + blur |
| `--color-text` / `--color-muted` / `--text-subtle` | Text scale | grey-900 / 500 / 400 | grey-dark-900 / 500 / 400 |

### Typography

- Typeface: Switzer (variable, 300–900) — already loaded globally
- Scale: `--text-xs` (13px, weight 500) · `--text-base` (15px, weight 400) · `--text-base-strong-weight` (600) · `--text-display` (reserved)
- Minimum: 13px — anywhere
- `font-variant-numeric: tabular-nums` for any aligning digits (meters, parameter values, timings)

### Dimension tokens

| Token | Value | Use |
|---|---|---|
| `--radius` | 0 | every interactive bordered element (sharp-edge direction) |
| `--input-height` | 32px | default height for inputs/buttons/selects/InputGroup |
| `--input-height-sm` | 24px | compact variant (badges, tags) |
| `--input-padding-x` | 0.6rem | horizontal padding for text inputs/buttons |
| `--border-container` | light: `none` / dark: `1px solid var(--color-border)` | optional outline for tinted containers (e.g. tab list) |

### Interaction patterns

- **Hover on bordered controls:** `border-color: var(--color-secondary)` + `background: color-mix(in srgb, var(--color-secondary) 8%, transparent)`. Transition 0.15s ease.
- **Focus on text inputs:** `border-color: var(--color-secondary)` (same as hover); `outline: none`.
- **Active/selected:** `color: var(--color-primary)` + orchid tint background. Never blue/grey.
- **Section-title convention (Options panel):** orchid for grouping headers (Parameters, States). Neutral `var(--color-text)` for input labels (Plugin, Test Tone, Level, Target Loudness).
- **Slider fill:** amber (`--color-secondary`). Thumb: neutral (`--color-text`) with surface ring.
- **Radix-controlled triggers** (Select/DropdownMenu) expose `--trigger-border` and `--trigger-radius` CSS variables — set on a parent to fuse a trigger into an `InputGroup` without modifying its markup.

### Icons

- Library: [`react-icons`](https://react-icons.github.io/react-icons/)
- Active sets: Tabler (`react-icons/tb`) + Lucide (`react-icons/lu`)
- Before introducing a third set, check existing imports. Prefer Tabler for new icons where both have an equivalent.

### Theme

- Zustand store holds `theme: 'light' | 'dark' | 'system'`; persists under localStorage key `stellarr.theme`
- `useSyncTheme()` (called in `App.tsx`) writes `data-theme="light"|"dark"` on `<html>` and subscribes to `prefers-color-scheme` changes when `'system'`
- UI toggle in header flips between resolved light/dark (skips `'system'` to avoid invisible transitions)

### Testing UI

- Runner: Vitest + JSDOM + `@testing-library/react`
- `cd ui && npm run test` — run all UI tests
- `cd ui && npm run test:watch` — watch mode
- Token tests use a `getVar()` helper (in `ui/src/design/__tests__/tokens.test.ts`) to resolve one level of `var()` indirection since JSDOM does not
