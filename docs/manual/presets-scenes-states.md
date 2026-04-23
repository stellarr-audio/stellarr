---
title: Presets, Scenes & States
description: Capture, recall, and switch rig snapshots.
sidebar:
  order: 7
---

Stellarr has a three-tier system for saving and recalling your sound:

```
Preset (file)
 └── Scenes (up to 16)
      └── Per-block States (up to 16 each)
```

## Presets

A preset is a `.stellarr` file that contains your entire rig: every block, connection, plugin configuration, scene, state, and preset-level MIDI mapping.

### Managing Presets

The preset controls are in the centre of the header bar:

- **Open** (folder icon) -- Browse for a `.stellarr` file to load.
- **Preset dropdown** -- Shows the current preset name. Click to switch between presets in the same folder. The bottom of the list has **+ New Preset** to start fresh.
- **Save** (bookmark icon) -- Quick-save to the current file. If no file exists yet, opens a Save As dialog.
- **Save dropdown** (chevron) -- Choose between **Save** (overwrite) and **Save As** (new file).

When you save, a checkmark briefly flashes to confirm.

### MIDI Preset Switching

Click the **Link icon** next to the preset dropdown to assign MIDI Program Change control. When assigned, each preset in the dropdown shows a **PC:N** tag indicating which Program Change value selects it (e.g., PC:0 for the first file, PC:1 for the second). See [MIDI](/docs/midi/) for details.

### Preset Folder

Stellarr remembers the last folder you saved to or loaded from. The preset dropdown shows all `.stellarr` files in that folder, letting you browse through them.

## Scenes

A scene captures **which state is active on every plugin block** and **whether each block is bypassed**. Think of scenes as "snapshots" of your entire rig configuration.

Use cases:
- **Scene 1: Clean** -- Amp on clean channel, chorus on, drive off
- **Scene 2: Crunch** -- Amp on drive channel, chorus off, boost on
- **Scene 3: Lead** -- Amp on high gain, delay on, boost on

### Managing Scenes

The scene controls are in the header bar next to the preset dropdown:

- **Scene dropdown** -- Shows the active scene name. Click to switch scenes.
- **Link icon** -- Assign a MIDI CC to switch scenes. When assigned, each scene in the dropdown shows a tag like **CC10 val:0**, **CC10 val:1**, etc., indicating which CC value triggers that scene.
- **+ Add Scene** -- Creates a new scene from the current state of all blocks.
- **Dots menu** (per scene) -- **Rename** or **Delete** a scene.

Every preset starts with one scene called "Scene 1". You can have up to 16 scenes.

### Stage Display

Above the grid, the active preset and scene names are displayed in large type — preset on the left, scene on the right, separated by a small diamond. The two names are colour-coded so the distinction is readable from across a stage. Both react instantly when you switch.

### How Scene Switching Works

Scene switching is **instant** -- there is no audio gap. Since all plugins are already loaded and running, Stellarr applies the new parameters directly without suspending audio processing.

When you switch to a different scene:
1. The current block states and bypass settings are saved into the outgoing scene.
2. Each block recalls the state index and bypass setting stored in the incoming scene.
3. All parameter changes (mix, balance, level, bypass) update in the UI instantly.

This means you can tweak settings, switch away, and switch back -- your changes are preserved until you save the preset.

## States

A state is a per-block snapshot of a plugin block's complete configuration:
- The plugin's internal state (all knobs, switches, loaded IRs, etc.)
- Block-level parameters: Mix, Balance, Level, Bypass, Bypass Mode

### Managing States

States are shown as numbered squares in the **States** section of the Options panel:

- **Click a square** to recall that state.
- **Click +** to add a new state (captures current settings).
- **Click X** on a square to delete it (must keep at least one).
- The **active state** has a white border.
- A state with **unsaved changes** shows a yellow background.

### Dirty States

When you change a parameter (mix, balance, etc.) or tweak the plugin, the active state becomes "dirty" -- indicated by a yellow background on its square. This tells you that the live settings differ from what's saved in that state.

Dirty states are preserved when switching between states. They are written to disk when you save the preset.

## How They Work Together

Here's a typical workflow:

1. **Set up your chain** on the Grid with plugin blocks.
2. **Configure each plugin** -- load an amp sim, set its drive, EQ, etc.
3. **Save a state** for each plugin's "clean" setting (State 1).
4. **Add a second state** (State 2) with a different plugin setting (e.g., high gain).
5. **Create Scene 1** ("Clean") -- all plugins on State 1, no blocks bypassed.
6. **Switch plugins to State 2**, bypass the chorus block.
7. **Create Scene 2** ("Drive") -- captures the current state indices and bypass.
8. **Save the preset** -- everything is stored in one `.stellarr` file.

Now you can switch between "Clean" and "Drive" with one click during a performance.
