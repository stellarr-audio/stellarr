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
