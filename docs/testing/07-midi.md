---
title: MIDI
sidebar:
  order: 7
---

Test whenever you touch MIDI mapping, MidiMapper, the MIDI page, or MIDI-triggered actions.

## Prerequisites

- A MIDI controller (hardware or software) that can send CC and Program Change messages
- Alternatively, use the CC sender on the MIDI page to simulate CC input

## Test Cases

### TC-MI-001: Assign CC to parameter

**Steps:**
1. Open the MIDI page
2. Add a mapping: channel Any, a CC number, target a block parameter (e.g. mix)
3. Send that CC from your controller

**Expected:** The parameter moves in response to the CC value.

### TC-MI-002: MIDI Learn

**Steps:**
1. Click the MIDI Learn button on a mapping or parameter
2. Move a knob/fader on your MIDI controller

**Expected:** The CC number and channel are auto-detected and assigned.

### TC-MI-003: Program Change for presets

**Steps:**
1. Assign Program Change to presets (Link icon on preset dropdown)
2. Send PC 0, PC 1, PC 2 from your controller

**Expected:** Presets switch to the corresponding index in the preset list.

### TC-MI-004: CC sender

**Steps:**
1. Open the MIDI page
2. Add a CC mapping to a parameter
3. Use the CC number input and value input to send a test CC

**Expected:** The mapped parameter responds to the injected CC value.

### TC-MI-005: MIDI monitor

**Steps:**
1. Open the MIDI page
2. Enable the MIDI monitor
3. Send various MIDI messages from your controller

**Expected:** Messages appear in real time with correct type, channel, and data values.

### TC-MI-006: Remove and clear mappings

**Steps:**
1. Add 3+ MIDI mappings
2. Remove one mapping
3. Clear all mappings

**Expected:** Individual removal works. Clear all removes everything. No orphan mappings.

### TC-MI-007: Mappings persist in preset

**Steps:**
1. Add MIDI mappings to parameters
2. Save the preset
3. Load a different preset (mappings should change)
4. Reload the original preset

**Expected:** Preset-level mappings are restored. Global mappings (preset change, scene switch) persist independently.

### TC-MI-008: Activity indicator

**Steps:**
1. Add a CC mapping
2. Send that CC from your controller

**Expected:** The mapping row briefly highlights to show activity.

### TC-MI-009: Assign MIDI to a block state via Learn

**Prerequisites:** A loaded preset with a plugin block that has at least 2 states (use the **+** button in the States section to add states if needed).

**Steps:**
1. Open the Options panel for a plugin block with multiple states.
2. Click the small link icon on the State 2 square.
3. In the dialog, click **Learn**, then send a CC from your controller (e.g. CC 64 with value 127).
4. Save.

**Expected:** The State 2 square's middle segment now shows `CC 64` in azure. The MIDI page lists a row reading `Block State (BlockName) -- State 2`.

### TC-MI-010: Per-state CC threshold

**Prerequisites:** TC-MI-009 completed (State 2 assigned to a CC, e.g. CC 64).

**Steps:**
1. Send the assigned CC at value 0 (release) -- state should not change.
2. Send the assigned CC at value 30 -- state should not change.
3. Send the assigned CC at value 63 -- state should not change.
4. Send the assigned CC at value 64 -- State 2 activates.
5. Send the assigned CC at value 127 -- State 2 stays active (no-op since already there).

**Expected:** Only values >= 64 trigger the state change. Lower values are silently ignored. The State 2 square gets the active amber outline once triggered.

### TC-MI-011: Scene precedence over per-state MIDI

**Prerequisites:** Block has State 1 and State 2; State 2 is mapped to a CC; one scene captured with State 1 active.

**Steps:**
1. Recall the scene -- State 1 active.
2. Send the State 2 CC -- State 2 active.
3. Recall the scene again -- State 1 active.
4. Send the State 2 CC again -- State 2 active.
5. Recall the scene one more time -- State 1 active.

**Expected:** Each scene recall re-asserts its captured state index. The per-state CC override does not stick across scene changes.

### TC-MI-012: Mapping cleanup on state and block delete

**Prerequisites:** Block with at least 3 states; State 2 mapped to CC A, State 3 mapped to CC B.

**Steps:**
1. Delete State 2 (click the X on its square).
2. Send CC B -- the state that was previously State 3 (now State 2) activates.
3. Open the **MIDI** page -- the surviving row reads `Block State (BlockName) -- State 2` (not State 3).
4. Delete the entire block from the grid.
5. Open the **MIDI** page -- no rows for that block remain.

**Expected:** Deleting a state drops its mapping and shifts higher-indexed mappings down by one. Deleting the block prunes every mapping that targeted it (state, bypass, mix, balance, level).
