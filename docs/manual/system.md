---
title: System
description: Audio device selection, buffer sizes, and performance.
sidebar:
  order: 8
---

The System tab manages plugin libraries, software updates, and app information.

## Software Updates

Stellarr checks for new versions on launch and once a day while you're running it. When a new version is available, an amber banner appears in the Software Updates section and a small amber dot appears on the System tab icon.

### Updates never install themselves

Installing an update restarts Stellarr. That's fine at your desk, but potentially catastrophic ten minutes before a gig. Stellarr never triggers an install without you pressing a button. Even if you leave the app running for weeks, a new release sits patiently in the Software Updates section until you decide it's a good time.

### The section

When no update is available:

- Status line reads "You're on vX.Y.Z, the latest version" with a green marker.
- **Check for Updates** stays available so you can poll on demand.

When an update is available:

- An amber banner shows the new version, release date, and download size.
- **View release notes →** opens the GitHub release page in your default browser so you can read what's changing.
- **Download & Install** commits to the full update flow. Stellarr downloads the release, verifies its signature, and stages the install on disk.

Once the download has completed the button becomes **Restart Now**, and a line underneath reads *"Update will install when you quit or restart Stellarr."* The install then happens either when you click Restart Now, or the next time you quit Stellarr for any reason — whichever comes first.

Clicking **Download & Install** is the point of no return: once the update is staged, it will install on the next termination. If you want to postpone, don't click the button yet.

### How updates are verified

Every Stellarr release is signed twice:

1. **Apple Developer ID** — macOS verifies this before running the app at all.
2. **EdDSA (Ed25519)** — Stellarr's own signing key, verified by the update framework before an installer is launched.

If either signature is wrong, the update is refused.

### Dev builds

If your window title reads `Stellarr Dev vX.Y.Z`, you're running a development build. Dev builds check a separate staging feed signed with a different key, and the Settings info panel shows a **Development build** marker. Dev builds can live in `/Applications` alongside a normal Stellarr install without overlapping.

### Rolling back

If a new release misbehaves, grab the previous DMG from the [Releases page](https://github.com/stellarr-audio/stellarr/releases) and install over the top. Your settings live in `~/Library/Application Support/Stellarr/` and are not wiped by reinstallation.

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
