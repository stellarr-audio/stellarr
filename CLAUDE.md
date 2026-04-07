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
- Build verification (`make build-ui` or `npx tsc --noEmit` for UI, `cmake --build` for engine)
- Run all tests before committing (`ctest --test-dir build --output-on-failure`)

## Documentation

- User manual lives in `docs/manual/` as numbered Markdown files served via Docsify
- When adding or significantly changing a user-facing feature, update the relevant manual page
- Match the existing tone: concise, second-person, no jargon without explanation
- Use New Zealand English spelling consistent with the rest of the project
- Do not create new manual pages without being asked — prefer extending existing ones
- Reference other pages with relative Markdown links (e.g., `[MIDI](05-midi.md)`)

## Git Workflow

- **Never commit directly to main** — main is protected
- Branch from main, push branch, create PR, merge via GitHub
- CI runs automatically on PRs and must pass before merge

### Before writing any code

1. Check the current branch (`git branch --show-current`)
2. If on `main` or an unrelated branch: stash any uncommitted work, switch to `main`, pull latest, and create a new branch
3. One branch per task — never mix unrelated changes on the same branch
4. If already on the correct branch for the current task, continue on it

### Committing and PRs

1. Stage only the files relevant to the task
2. Push the branch and create a PR (draft if the work is untested or in progress)
3. Wait for CI to pass before requesting merge
4. After merge, delete the remote branch and switch back to `main` locally

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

## Build Commands

- `make dev` — build UI + engine (no tests), clears WebView cache
- `make debug` — build UI + engine with tests
- `make release` — optimised build
- `make run` / `make run-debug` / `make run-release` — build and launch
- `make test` — build with tests and run them
- Always run `make build-ui` or `npx tsc --noEmit` to catch TypeScript errors, not just `make`

## Architecture Notes

- Engine (C++): JUCE AudioProcessorGraph for audio routing
- UI (React/TS): Zustand store, Radix UI components, CSS Modules
- Bridge: UI `sendEvent` -> C++ `handleEvent` -> `emitToJs` -> Zustand store
- Tests: C++ test executables gated behind `BUILD_TESTING` CMake flag
- Declarations in headers, implementations in .cpp files
