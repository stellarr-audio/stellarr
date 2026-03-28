# Stellarr

Open-source guitar signal processing application built with JUCE (C++) for the audio engine and React + TypeScript for the UI, connected via the JUCE WebView bridge.

## Prerequisites

- CMake 3.24+
- C++ compiler with C++20 support (Xcode Command Line Tools on macOS)
- Node.js 18+
- npm

## Project Structure

- `engine/` — C++ audio engine, plugin editor, and WebView bridge
- `engine/test/` — Audio processing tests
- `ui/` — React + TypeScript frontend (Vite, Zustand)

## Getting Started

### 1. Install UI dependencies

```
cd ui
npm install
```

### 2. Build the UI

```
npm run build
```

### 3. Configure and build the C++ project

From the project root:

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The first configure fetches JUCE via git and builds the `juceaide` tool. This takes several minutes on the first run.

### 4. Run the standalone app

```
open build/Stellarr_artefacts/Debug/Standalone/Stellarr.app
```

### 5. Run tests

```
ctest --test-dir build --output-on-failure
```

## Build Targets

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
