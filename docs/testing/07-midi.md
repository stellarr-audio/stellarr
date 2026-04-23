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
