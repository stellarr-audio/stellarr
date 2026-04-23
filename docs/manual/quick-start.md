---
title: Quick Start
description: Get a rig running in a few minutes.
sidebar:
  order: 2
---

Get up and running in under five minutes.

## 1. Launch Stellarr

Open the app. You'll see the Grid page with two default blocks: **INP** (Input) and **OUT** (Output).

## 2. Set Up Audio

Click the options button in the app's title bar and select **Audio/MIDI Settings**. Choose your audio interface for input and output.

## 3. Add a Plugin Block

Click on an empty cell between the Input and Output blocks. Select **Plugin** from the menu. A new plugin block appears on the grid.

## 4. Connect the Chain

- Drag from the Input block's right edge to the Plugin block's left edge.
- Drag from the Plugin block's right edge to the Output block's left edge.

Audio now flows: Input > Plugin > Output.

**Tip:** If you place a new block on an existing connection wire, Stellarr automatically splices it into the chain for you.

## 5. Load a Plugin

Click the Plugin block to select it. In the Options panel on the right:
1. Open the **Plugin** dropdown.
2. Search for your plugin by name.
3. Click to load it.

Click the gear icon to open the plugin's own editor window.

## 6. Play

Plug in your guitar (or enable **Test Tone** on the Input block to hear a demo melody) and play through your chain.

## 7. Save Your Preset

Click the save icon in the header bar, or use **Save As** from the dropdown to choose a location. Your entire rig is saved as a `.stellarr` file.

## What's Next?

- **[Grid](/docs/grid/)** -- Learn about blocks, connections, and the signal chain
- **[Presets, Scenes & States](/docs/presets-scenes-states/)** -- Save and recall configurations
- **[MIDI](/docs/midi/)** -- Control everything with your MIDI hardware
