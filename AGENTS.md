# AGENTS.md — Codex review playbook

This file is read by automated reviewers (currently OpenAI Codex Cloud) when reviewing pull requests against this repository.

**Project rules live in [`CLAUDE.md`](./CLAUDE.md). That file is the source of truth.** This file is a thin review playbook — it points at the relevant CLAUDE.md sections and adds Codex-specific guidance: how to triage by severity, which architectural hot spots demand a higher bar, and which code-quality patterns to treat as standing review passes.

---

## How to review a PR

1. Read the diff against the rules in `CLAUDE.md`. The most load-bearing sections for review:
   - **Audio thread safety** — hard rules for anything in `engine/` that touches `processBlock` or the `AudioProcessorGraph`.
   - **Design system (UI)** — golden rules for anything in `ui/src/`.
   - **Architecture Notes / Key paths** — orient yourself to where the changed code sits.
   - **Git Workflow / Commit messages** — conventional commits, branch naming, no Co-Authored-By trailers.
2. Apply the severity rubric below.
3. Run the code-quality lenses as a standing pass before concluding "no issues".
4. If the file sits in an architectural hot spot, raise the bar: small mistakes there have outsized cost.

## Severity rubric

| Level | Meaning | Examples |
|---|---|---|
| **P1** | Must fix before merge. Correctness, real-time safety, security, data loss, regressions. | Heap allocation in `processBlock`, graph mutation from audio thread, off-by-one in tuner threshold, broken auth gate, silent data corruption. |
| **P2** | Should fix. Performance, UX precision, accessibility, observable regressions that aren't crashes. | Sub-Hz precision lost in tuner readout, N+1 query, missing aria-label, suboptimal lock granularity. |
| **P3** | Nice to fix. Style, naming, dead code, DRY, comments. | Duplicated ternary across call sites, empty `useEffect`, unused import, redundant comment. |

Always tag comments with the level. P3 should be terse — one line each.

## Code-quality lenses

Patterns that reliably indicate a regression. Treat them as a standing review pass — they sit alongside the architecture and domain checks, not after them.

- **DRY across call sites.** Logic that should live in one place but has been spread across multiple style props, branches, or callbacks — typically the same ternary or object literal repeated. Hoist it.
- **Dead React surface.** Effects that do nothing (`useEffect(() => {}, [])`, callbacks with empty bodies), imports that have no effect on output, locals that survive a refactor but are no longer read. TypeScript strict catches some; flag what slips through.
- **Numeric precision regressions** in user-facing readouts. Anywhere a value is shown for tuning, metering, or timing, sub-unit precision can be load-bearing — `Math.floor` / `Math.round` substitutions for `toFixed` (or vice versa) drop information without obvious symptom.
- **Design-system bypass.** Hardcoded hex in `.module.css`, bespoke `<input>` / `<button>` styling, ad-hoc heights instead of `var(--input-height)` — all signal that someone has reached around `ui/src/design/tokens.css` and `ui/src/components/common/` rather than extending them. Flag the bypass and propose the token / primitive that should have been used.
- **Asset duplication.** Anything copied into `ui/src/` that already lives under `assets/` should be a symlink or an import, never a copy. Drift between copies is a future bug.

## Architectural hot spots

Areas where a regression is disproportionately costly. Code reachable from these surfaces inherits the same higher bar.

- **Real-time audio path.** Anything called from `StellarrProcessor::processBlock`, block `processBlock` overrides in `engine/blocks/*.cpp`, or any audio callback. Audio-thread rules in CLAUDE.md (no allocation, no graph mutation, no blocking lock, no logging, no I/O) are absolute here.
- **Message-thread / audio-thread boundary.** `engine/StellarrBridge.cpp`, `engine/bridge/*`, and anything in `engine/PluginManager.cpp` that touches plugin loading. This is where races, priority inversions, and "works on my machine" bugs live.
- **UI ↔ C++ bridge contract.** `ui/src/bridge/index.ts` and `ui/src/store/index.ts` on one side, `engine/StellarrBridge.cpp` and `engine/bridge/*` on the other. Bridge events flow `sendEvent` → `handleEvent` → `emitToJs` → Zustand. A PR that adds an event on only one side of this triad is structurally incomplete.
- **Design system surface.** `ui/src/design/tokens.css`, `ui/src/styles/variables.css`, and the primitives in `ui/src/components/common/`. Changes here ripple across every screen; check for token drift, role inversion (orchid ↔ amber), and silent removal of tokens still referenced elsewhere.

## Domain rules to enforce

These are project-wide invariants that aren't visible from the diff alone — confirm the change respects them.

- **Graph mutation batching.** Modifications to the `AudioProcessorGraph` must use `UpdateKind::none` for the batch and end with a single `rebuildGraph()`. Multiple intermediate `UpdateKind::full` calls inside a loop are a bug, even if the result looks correct.
- **Pre-suspension preparation.** When swapping plugin instances, the new instance must be fully constructed and `prepareToPlay`-ready before `suspendProcessing(true)` is called. Suspension windows must be minimised.
- **Atomicity of cross-thread state.** Data shared between audio and message threads uses `std::atomic`. Audio-thread access to non-atomic shared state, or use of blocking locks where `SpinLock::ScopedTryLockType` is required, is a P1 defect.
- **NZ English in user-facing copy.** `colour` not `color`, `centre` not `center`, etc. Code identifiers as written by JUCE/React stay untouched.
- **No emoji in any output, log, or generated content.**

## What not to flag

Don't waste review surface on:

- Conventional Commits formatting (CI/lint catches this).
- TypeScript errors (`tsc --noEmit` catches them; flag only if the diff suppresses an error with `// @ts-ignore` etc.).
- Trivial whitespace / formatting that the editor handles.
- "Could be more functional" rewrites of imperative code that already works.
- Speculative future-proofing requests ("what if we want to support X later?").

## When in doubt

If the diff genuinely has nothing to flag at P1/P2, leave a thumbs-up reaction and stop. Don't manufacture P3 comments to look thorough.
