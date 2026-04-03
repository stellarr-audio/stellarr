# MIDI

Stellarr supports full MIDI control for live performance and studio use.

## How MIDI Works in Stellarr

MIDI in Stellarr has two layers:

1. **Pass-through** -- MIDI from your controller flows through the signal chain to your plugins. Plugins that respond to MIDI (synths, harmonisers, etc.) receive notes and CCs directly.
2. **Stellarr control** -- You can map MIDI CCs to control Stellarr's own parameters: block bypass, mix, balance, level, scene switching, and more.

Stellarr intercepts mapped CCs before they reach plugins. Unmapped MIDI passes through to plugins unmodified.

## Assigning MIDI Controls

### From Block Parameters

Every controllable parameter has a **MIDI** button next to it:

1. Click the **MIDI** button next to any parameter (Mix, Balance, Level, Bypass).
2. A dialog opens with:
   - **CC Number** -- Type a CC number (0--127), or use the **Learn** button.
   - **Channel** -- Choose a specific channel (1--16) or **Any**.
3. Click **Save** to create the mapping.

### MIDI Learn

Instead of typing a CC number:

1. Click the **Learn** button (mixer icon) in the MIDI dialog.
2. The button highlights to show it's listening.
3. Move a knob or press a button on your MIDI controller.
4. Stellarr auto-detects the CC number and channel.
5. Click **Save** to confirm.

### What Can Be Mapped

| Target | Description | CC Behaviour |
|--------|-------------|--------------|
| Block Bypass | Toggle block on/off | CC >= 64 = on, < 64 = off |
| Block Mix | Wet/dry blend | CC 0--127 maps to 0--100% |
| Block Balance | Stereo balance | CC 0--127 maps to L100--R100 |
| Block Level | Output gain | CC 0--127 maps to -60 to +12 dB |
| Scene Switch | Recall a scene | CC value = scene index |
| Preset Change | Switch presets | Program Change message |
| Tuner Toggle | Enable/disable tuner | CC >= 64 = on, < 64 = off |

## MIDI Page

The **MIDI** tab shows all active mappings in a table:

- **In** -- Activity indicator (diamond flashes green when a matching CC is received)
- **CC** -- The CC number (or "PC" for Program Change)
- **Ch** -- MIDI channel (1--16 or "Any")
- **Target** -- What the mapping controls
- **X** -- Delete the mapping

Click any row to **edit** the mapping (change CC number or channel).

Use **Clear All** to remove all mappings at once.

### Global vs Preset Mappings

- **Preset Change** and **Tuner Toggle** mappings are **global** -- they persist across all presets and are stored in app settings.
- All other mappings are **preset-level** -- they are saved inside the `.stellarr` file and change when you switch presets.

## MIDI Monitor

The right sidebar on the MIDI page shows a live log of all incoming MIDI events:

- **CC** messages: `CC Ch1 CC7=64`
- **Note** messages: `Note On Ch1 Note60 Vel100`
- **Program Change** messages: `PC Ch1 #5`

The monitor auto-activates when you open the MIDI tab and deactivates when you leave.

## CC Sender

Below the monitor is a **Send CC** panel for testing your mappings without hardware:

1. Set the **CC#** (0--127).
2. Set the **Channel** (1--16).
3. Adjust the **Value** slider (0--127).
4. Click **Send**.

The CC is injected into Stellarr's audio processing as if it came from a real MIDI device. The corresponding mapping's activity diamond will flash, and the parameter will change.

## MIDI Device Selection

To choose which MIDI devices Stellarr listens to, click the options button in the app's title bar and select **Audio/MIDI Settings**. Enable the MIDI inputs you want to use.
