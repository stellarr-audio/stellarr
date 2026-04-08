# Preset Switching

Test whenever you touch preset loading, `restoreSession`, `clearGraph`, plugin lifecycle, or `StellarrProcessor` graph management.

This is historically the most crash-prone area. The engine suspends the audio graph, tears down all blocks, creates new plugin instances, rebuilds connections, and resumes -- all on the message thread while the audio thread is running.

## Prerequisites

- 2+ presets saved in a preset directory, each with different plugin chains
- At least one "heavy" preset with 3+ plugins
- Audio playing through the app (e.g. guitar input or test tone)

## Test Cases

### TC-PS-001: Switch between heavy presets

**Steps:**
1. Load a preset with 3+ plugins in the chain
2. Verify audio is flowing (output meter shows signal)
3. Switch to a different heavy preset via the preset dropdown

**Expected:** No crash. Audio gap is brief (a click or short silence, not seconds). All plugins in the new preset produce audio.

### TC-PS-002: Rapid preset switching

**Steps:**
1. Load any preset
2. Click through 3+ different presets in quick succession (as fast as you can click)

**Expected:** No crash, no hang. The final preset loads correctly and produces audio.

### TC-PS-003: Switch with plugin editor open

**Steps:**
1. Load a preset with at least one plugin
2. Open the plugin's editor window (gear icon in Options panel)
3. Switch to a different preset

**Expected:** Plugin editor window closes. New preset loads without crash.

### TC-PS-004: Switch with test tone playing

**Steps:**
1. Enable the test tone on the Input block
2. Switch to a different preset

**Expected:** Test tone stops (new preset has its own Input block). New preset loads cleanly.

### TC-PS-005: New session clears graph

**Steps:**
1. Load a heavy preset
2. Choose New Preset from the save menu

**Expected:** All plugin blocks are removed. Graph shows only Input and Output blocks. No crash.

### TC-PS-006: Save and reload preserves state

**Steps:**
1. Load a preset with plugins
2. Adjust a plugin's parameters (open its editor, change a knob)
3. Save the preset (Cmd+S)
4. Switch to a different preset
5. Switch back to the original preset

**Expected:** The plugin parameter changes from step 2 are preserved.

### TC-PS-007: MIDI-triggered preset change

**Steps:**
1. Assign Program Change to presets (Link icon on preset dropdown)
2. Send a PC message from your MIDI controller

**Expected:** Preset switches to the corresponding index. No crash.

**Notes:** Requires a MIDI controller or software MIDI source.
