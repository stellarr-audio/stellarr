---
title: The Grid
description: Blocks, connections, and how signal flows on the grid.
sidebar:
  order: 3
---

The Grid is where you build your signal chain. It's a visual workspace where blocks are placed and connected with wires.

## Adding Blocks

Click any empty cell on the grid. A menu appears with three options:

- **Input** -- Audio input from your interface
- **Output** -- Audio output to your speakers
- **Plugin** -- Hosts a VST3 or AU plugin

Each block appears as a square on the grid showing its type abbreviation (INP, OUT, PLG) or a custom name you assign.

## Moving Blocks

Drag any block to reposition it on the grid.

## Connecting Blocks

Audio and MIDI flow through connections between blocks:

1. Hover over the **right edge** (output side) of a block -- the edge thickens to indicate it is interactive.
2. Click and drag from the thickened edge toward another block.
3. Release on the **left edge** (input side) of the destination block.

A wire appears showing the connection. Audio and MIDI both travel through the same connection. When a block has multiple connections on one side, each wire is spaced equidistantly along the edge.

### Splice Insertion

If you place a new block directly on an existing connection wire, Stellarr automatically:
1. Removes the original connection.
2. Connects the source to your new block.
3. Connects your new block to the destination.

This saves you from manually rewiring.

### Disconnecting

Two ways to remove connections:

- **Click on a wire** -- hover over a connection line (it turns dashed to show it is interactive), then click to see a "Disconnect" option.
- **Click on a block edge** -- click the left or right edge of a block that has connections. A menu lists all connections on that side, each with a disconnect button.

## Copying and Pasting Blocks

Hover over a block to reveal the **copy icon** in the top-left corner. Click it to copy the block (including its plugin, states, and parameters) to the clipboard.

To paste, right-click on any empty cell and select **Paste** from the context menu. The pasted block gets a fresh identity but retains all settings from the original. If the original plugin is available, it is loaded automatically; otherwise, the block appears with a missing plugin indicator.

## Removing Blocks

Hover over a block and click the **X** icon that appears in the corner.

## Block Appearance

Each block displays:

| Area | Content |
|------|---------|
| Top | Plugin format badge (VST3, AU) or test tone icon |
| Middle | Block name (INP, OUT, PLG, or custom 3-char name) |
| Bottom | Plugin name (if a plugin is loaded) |

### Block Colors

Each block has a colour that tints its border. By default:
- Input and Output blocks are **slate**
- Plugin blocks are **blue**

You can change a block's colour from the Options panel using the colour picker.

### Bypass Indicator

When a block is bypassed, its border changes to a **dashed line**.

### Missing Plugin Indicator

If a preset references a plugin that is no longer installed, the block shows a **yellow warning icon** and an amber border with the text "Missing: PluginName". You cannot open the plugin editor for a missing block, but the block remains in the grid so you can see where it was in the chain.

## Connection Route Highlighting

When you select or hover over a block, Stellarr highlights the **live signal routes** passing through it:

- Connections on a complete path from an Input block through the selected block to an Output block are shown in **amber** at full opacity.
- All other connections fade to **15% opacity** so the active route stands out.
- Blocks with bypass mode set to **Mute** are treated as dead ends -- signal stops there, and connections beyond a muted block are not highlighted.

This makes it easy to trace your signal flow at a glance, even in complex multi-path chains. Diamond and convergent topologies are fully supported.

## Preset and Scene Display

Above the grid, the current preset name and active scene name are shown in large text, separated by a diamond. This is visible from across the room during a performance.

## Grid Size

You can resize the grid to fit the rig you're building.

- Hover the edges of the grid to reveal the **add row** / **add column** controls on the right and bottom edges.
- Hover a single row or column to reveal a **delete** affordance for that line (if it's empty).
- Bounds: 1–20 columns, 1–12 rows.

Changes apply immediately and are saved with the preset.

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Space | Toggle bypass on the selected block |
