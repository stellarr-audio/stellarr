# Stellarr

Open-source guitar signal processing application built with JUCE (C++) for the audio engine and React + TypeScript for the UI, connected via the JUCE WebView bridge.

## Prerequisites

- CMake 3.24+
- C++ compiler with C++20 support (Xcode Command Line Tools on macOS)
- Node.js 18+
- npm

## Quick Start

```
make build    # install npm deps, build UI, configure + build C++
make run      # launch the standalone app
make test     # run audio processing tests
```

The first build fetches JUCE via git and compiles the `juceaide` tool. This takes several minutes on the first run.

## Project Structure

- `engine/` — C++ audio engine, plugin editor, and WebView bridge
- `engine/test/` — Audio processing tests
- `ui/` — React + TypeScript frontend (Vite, Zustand)

## Make Targets

| Target | Description |
|---|---|
| `make build` | Full build: npm install, UI build, CMake configure + compile |
| `make build-ui` | Install npm deps and build the React app only |
| `make build-cpp` | CMake configure and compile only |
| `make test` | Build and run all tests |
| `make run` | Build and launch the standalone app |
| `make clean` | Remove build/, ui/dist/, and ui/node_modules/ |

## Build Outputs

The CMake build produces three plugin formats from the same codebase:

- **Standalone** — runs as a regular macOS application
- **VST3** — installed to `~/Library/Audio/Plug-Ins/VST3/`
- **AU** — installed to `~/Library/Audio/Plug-Ins/Components/`

## Development

### UI hot reload

Run the Vite dev server for rapid UI iteration:

```
cd ui
npm run dev
```

Then rebuild the UI (`npm run build`) before launching the plugin to see changes in the WebView.

### WebKit Inspector

In debug builds, right-click anywhere in the plugin window and select "Inspect Element" to open the WebKit Inspector. Bridge activity is logged to the console with `[Bridge]` prefix.

## Licence

MIT
