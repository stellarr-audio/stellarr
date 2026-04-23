---
title: System
description: Audio device selection, buffer sizes, and performance.
sidebar:
  order: 8
---

The System tab manages plugin libraries and shows app information.

## Plugin Libraries

Stellarr discovers VST3 and Audio Unit plugins by scanning directories on your system.

### Default Directories

These are scanned automatically and cannot be removed:

| Format | macOS Path |
|--------|-----------|
| VST3 | `~/Library/Audio/Plug-Ins/VST3` |
| VST3 | `/Library/Audio/Plug-Ins/VST3` |
| AU | `~/Library/Audio/Plug-Ins/Components` |
| AU | `/Library/Audio/Plug-Ins/Components` |

### Adding Custom Directories

If your plugins are installed elsewhere:

1. Click **Add Directory**.
2. Browse to the folder containing your plugins.
3. Click **Scan Now** to discover them.

### Scanning

Click **Scan Now** to rescan all configured directories. This picks up newly installed plugins or changes since the last scan.

## Discovered Plugins

A list of all plugins Stellarr has found, showing:

- **Plugin name**
- **Manufacturer**
- **Format** (VST3 or AU)

If no plugins appear, check that your plugin directories are correctly configured and click **Scan Now**.

## Performance Meters (Footer)

The footer of the app runs three live meters:

- **CPU** — Audio processing load as a percentage. Green below ~40%, amber 40–70%, red above 70%.
- **IN** — Input level from your audio interface in **dBFS**. Helps you set input gain before the first block.
- **OUT** — Output level after the final block, in **dBFS**. Red when the signal clips above 0 dBFS.

When a plugin block is selected, the footer also shows that block's **live LUFS** reading. See [Block Options](/docs/block-options/) for setting per-block loudness targets.

## Theme

Click the sun/moon icon in the header to switch between **Dark** and **Light** themes. Your choice is persisted across sessions. If you've never clicked the toggle, Stellarr follows your system appearance automatically.

## Audio & MIDI Settings

Click the options button in the app's title bar and select **Audio/MIDI Settings** to configure:

- Audio input and output devices
- Sample rate and buffer size
- MIDI input devices
