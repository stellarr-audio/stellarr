# Block Options

Select any block on the grid to see its options in the right-side panel.

## Block Header

The top of the options panel shows:

- **Colour picker** -- Click the coloured square to choose from 16 theme swatches (8 hues, each with a regular and light shade).
- **Block name** -- Shows the 3-character abbreviation (INP, OUT, PLG) or your custom name. Click the pencil icon to rename (max 3 characters).
- **Active toggle** -- (Plugin blocks only) An on/off switch to bypass the block. When bypassed, the block's border becomes dashed and audio passes through unprocessed.
- **MIDI button** -- (Next to the bypass toggle) Assign a MIDI CC to control bypass remotely.

## Input Block Options

- **Test Tone** -- Toggle on to play a built-in pentatonic melody. Useful for testing your chain without plugging in a guitar.
- **Level** -- Output level from -60 dB to +12 dB. Double-click the slider to reset to 0 dB.

## Output Block Options

- **Level** -- Output level from -60 dB to +12 dB.

## Plugin Block Options

### Plugin Selection

- **Plugin dropdown** -- Searchable list of all discovered VST3 and AU plugins. Search by name or manufacturer. Each plugin shows a coloured format badge (VST3, AU).
- **Plugin editor** -- Click the gear icon to open the plugin's native GUI in a separate window.

### Parameters

These parameters control how the block's audio is processed:

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Mix | 0--100% | 100% | Wet/dry blend. 0% = fully dry (unprocessed), 100% = fully wet. |
| Balance | L100--R100 | C (centre) | Stereo balance. Attenuates the opposite channel as you move from centre. |
| Level | -60 to +12 dB | 0 dB | Output gain applied after mix and balance. |
| Bypass Mode | Thru / Mute | Thru | What happens when bypassed. **Thru** passes audio unchanged. **Mute** silences the block completely. |

Each parameter has a **MIDI** button for assigning a CC controller. Shows the assigned CC number when mapped.

**Tip:** Double-click any slider to reset it to its default value.

### States

Plugin blocks can save up to **16 states**. Each state captures:
- All of the plugin's internal settings
- Mix, Balance, Level, Bypass, and Bypass Mode

**Numbered squares** represent each state:
- Click a square to **recall** that state.
- The active state has a **white border**.
- A state with unsaved changes shows a **yellow background**.
- Click the **X** on a state to delete it (must keep at least one).
- Click the **+** button to add a new state.

States are saved when you save the preset. See [Presets, Scenes & States](07-presets-scenes-states.md) for the full picture.
