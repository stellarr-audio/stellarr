# System

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

## System Stats

In the top-right corner of the header bar, two bars show real-time performance:

- **CPU** -- Audio processing load as a percentage. Colour changes from green (< 40%) to yellow (40--70%) to red (> 70%).
- **OUT** -- Output level meter showing peak amplitude in **dBFS** (decibels relative to full scale). Green below -6 dB, yellow from -6 to 0 dB, red above 0 dB. When the output exceeds 0 dBFS the signal is **clipping** -- the value flashes red. Shows "-inf" at silence.

## Audio & MIDI Settings

Click the options button in the app's title bar and select **Audio/MIDI Settings** to configure:

- Audio input and output devices
- Sample rate and buffer size
- MIDI input devices
