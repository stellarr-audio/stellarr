---
title: MIDI
description: Map parameters to controllers, banks, and footswitches.
sidebar:
  order: 5
---

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

### From the Header Bar

The **Preset** and **Scene** dropdowns each have a **Link icon** button on their right edge:

- **Preset Link icon** -- Opens the MIDI assign dialog in **Program Change** mode. The CC field is hidden; the mapping responds to MIDI Program Change messages, where the PC value maps directly to the preset index in the active folder.
- **Scene Link icon** -- Opens the MIDI assign dialog for scene switching. Assign a CC number, and the CC value selects the scene index (value 0 = first scene, value 1 = second, etc.).

When a mapping is active, the Link icon fills with the brand accent to indicate it is assigned.

### From the Tuner Tab

The **Tuner** title has its own **Link icon** that opens the MIDI assign dialog in binary mode. Assign a CC and a threshold, and the tuner activates when the CC value crosses the threshold. Sending the toggle also switches the active view to the Tuner tab automatically, so a foot switch press surfaces the tuner without you reaching for the trackpad. Releasing (CC value below threshold) returns you to the Grid tab.

### What Can Be Mapped

| Target | Description | CC Behaviour |
|--------|-------------|--------------|
| Block Bypass | Toggle block on/off | CC ≥ threshold = on, below = off (threshold per-mapping; default 64) |
| Block Mix | Wet/dry blend | CC 0--127 maps to 0--100% |
| Block Balance | Stereo balance | CC 0--127 maps to L100--R100 |
| Block Level | Output gain | CC 0--127 maps to -60 to +12 dB |
| Block State | Recall a state on a single block | CC ≥ threshold fires; below ignored (threshold per-mapping; default 64) |
| Scene Switch | Recall a scene | CC value = scene index |
| Preset Change | Switch presets | Program Change value = preset index |
| Tuner Toggle | Enable/disable tuner -- automatically switches to the Tuner tab when on | CC ≥ threshold = on, below = off (threshold per-mapping; default 64) |

### Per-State MIDI Mapping

Each plugin block state can be triggered by its own CC. This is the canonical way to map a footswitch to an amp-channel switch, IR slot select, or any param snapshot you want recalled live.

To assign:

1. Open the **Options** panel for a plugin block.
2. In the **States** row, click the small link icon between the state number and the delete cross.
3. Use **Learn** to capture an incoming CC, or enter the channel and CC manually.

The state activates whenever the assigned CC arrives with a value at or above the mapping's threshold (default 64; configurable per mapping in the assign dialog). Values below the threshold are ignored, so a standard footswitch (sending 127 on press, 0 on release) latches the state cleanly.

**How this interacts with scenes**

Scenes still capture every block's active state index. When you recall a scene, every block snaps to the state that scene saved. After that, sending a per-state CC overrides one block until the next scene recall. The override does not stick across scene changes.

## CC Range and Curves

For continuous targets (Block Mix, Block Balance, Block Level), each MIDI mapping can be shaped via a per-mapping CC range, parameter range, and response curve. Open the assign dialog and look at the **Shaping** section.

### Anchors

Two anchor inputs define the affine map between the CC and the parameter:

- **Min CC** -- the CC value at the bottom anchor.
- **Min Mix / Balance / Level** -- the parameter value at that CC.
- **Max CC** -- the CC value at the top anchor.
- **Max Mix / Balance / Level** -- the parameter value at that CC.

CC values inside the range map smoothly to the parameter range. Outside the range, the parameter clamps to the nearest endpoint.

To **invert** the response (e.g. CC 0 maps to 100% Mix, CC 127 maps to 0%), set the Min row's parameter value higher than the Max row's. The CC column always runs low to high; inversion lives in the parameter column.

### Curves

The **Curve** dropdown picks the shape of the response between the two anchors:

- **Linear** (default) -- straight line from Min to Max.
- **Log** -- concentrated at low values; small CC changes near Min produce large parameter changes.
- **Exp** -- concentrated at high values; the inverse of Log.
- **S-curve** -- smooth transition with steep middle and flatter ends.

The mapping preview below the inputs draws the full curve between the anchors. Hover the preview to read the precise CC value and the parameter it maps to.

## Trigger threshold

For binary targets (Block Bypass, Block State), each MIDI mapping has a per-mapping **Threshold** that decides where the CC value flips the action. Open the assign dialog and look at the **Trigger** section.

- **Block Bypass** -- the block engages when CC ≥ threshold and bypasses when CC < threshold. The default 64 matches a standard footswitch (127 on press, 0 on release).
- **Block State** -- the state recalls when CC ≥ threshold; lower values are ignored.

> **Polarity change (v0.16.0):** in earlier builds, `CC ≥ 64` would *bypass* a block (turn the effect off). It now *engages* the block (turn the effect on), matching the docs and footswitch convention. Existing Bypass mappings will trigger the opposite of the previous behaviour -- check your saved presets after upgrading.

The Threshold slider runs from CC 1 to 127 with the active region (right of the handle) tinted amber. The rule line beside the label spells out the resulting behaviour, e.g. `ON ≥ CC 80` for a Bypass mapping or `RECALL ≥ CC 80` for a State mapping.

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

## MIDI Sender

Below the monitor is a **Send** panel for testing your mappings without hardware. Toggle between **CC** and **PC** modes via the tabs.

**CC mode** -- inject a Control Change message:

1. Set the **CC#** (0--127).
2. Set the **Ch** (1--16).
3. Set the **Val** (0--127).
4. Click **Send**.

**PC mode** -- inject a Program Change message:

1. Set the **Prog** number (0--127).
2. Set the **Ch** (1--16).
3. Click **Send**.

Both modes inject into Stellarr's audio processing as if from a real MIDI device. CC sends flash the matching mapping's activity diamond and update the parameter; PC sends switch presets when the program number matches a preset's index in the active folder. (Scene switching is CC-based -- use CC mode for that.)

The MIDI Sender is also available as a floating panel from the Grid toolbar (when [developer mode](/docs/system/) is on) so you can test without leaving the Grid view. The floating panel is draggable and remembers where you left it; if you reopen Stellarr at a smaller window size, the panel snaps back inside the visible area on the first frame so you never have to chase a titlebar offscreen.

## MIDI Device Selection

To choose which MIDI devices Stellarr listens to, click the options button in the app's title bar and select **Audio/MIDI Settings**. Enable the MIDI inputs you want to use.
