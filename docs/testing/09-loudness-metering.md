---
title: Loudness Metering
sidebar:
  order: 9
---

Test whenever you touch the LUFS DSP (`dsp/KWeighting.h`, `dsp/LoudnessMeter.h`), the meter plumbing in `Block`/`OutputBlock`, or the Signal section in the options panel.

## Prerequisites

- A preset with an Input, at least one Plugin block, and an Output
- An audio source (test tone on the Input block works for all cases)

## Test Cases

### TC-LM-001: Header meter tracks output loudness

**Steps:**
1. Start Stellarr with no audio playing
2. Observe the OUT meter in the header
3. Enable the test tone on the Input block

**Expected:** OUT meter is at silence floor when idle; rises to a steady reading when the test tone plays. Reading is stable (not jumpy) because the short-term window averages over 3 s.

### TC-LM-002: Selecting a block shows its history strip

**Steps:**
1. Select the Input block
2. Observe the Signal section in the options panel

**Expected:** A 30-second rolling history graph appears. Y-axis shows 0, -18, -30, -60 LUFS labels with dotted grid lines. The polyline plots the block's recent loudness.

### TC-LM-003: Switching selection stops measurement on the previous block

**Steps:**
1. Select a Plugin block; let the history fill with a visible polyline
2. Select a different Plugin block

**Expected:** The first block's history stops updating (it is no longer measured). The second block starts accumulating its own history from scratch. Only the selected block plus the Output block are actively measured.

### TC-LM-004: Meter runs in every bypass mode

**Steps:**
1. Select a Plugin block
2. Enable the test tone so there is signal flowing
3. Cycle through each bypass mode with bypass ON: Thru, Mute In, Mute Out, Mute, Mute FX In, Mute FX Out

**Expected:** The history strip updates in every mode. Thru and Mute FX In (with wet tail blended to dry) should read similar to the input; Mute / Mute Out should drop to silence floor; Mute In reads the plugin's tail (if any) decaying.

**Notes:** Regression guard — earlier versions skipped the meter entirely when bypassed, so Mute/Mute Out appeared frozen instead of dropping to floor.

### TC-LM-005: Target loudness line appears on Output block

**Steps:**
1. Select the Output block
2. Enable the target toggle; enter a value (e.g. -18)
3. Observe the history strip

**Expected:** A solid horizontal line appears at -18 LUFS across the graph. The header OUT meter changes colour based on proximity to the target (under/over).

### TC-LM-006: Target loudness persists across preset save/load

**Steps:**
1. Set Output target to -14 LUFS
2. Save the preset
3. Switch to another preset, then switch back

**Expected:** Target value is restored to -14. Unsetting the target and saving should omit `targetLufs` from the JSON (inspect the `.stellarr` file to confirm).

### TC-LM-007: Target loudness defaults to off for presets without the field

**Steps:**
1. Open a preset that pre-dates this feature (no `targetLufs` key)
2. Select the Output block

**Expected:** Target toggle is off; no target line drawn; no crash or NaN display.

### TC-LM-008: Momentary vs short-term window

**Steps:**
1. Open the window selector popup (click the OUT meter in the header)
2. Play the test tone; switch between Momentary (400 ms) and Short-term (3 s)

**Expected:** Momentary reacts faster to level changes; short-term is smoother. Starting and stopping the test tone should visibly differ in responsiveness between the two modes.

### TC-LM-009: Average line on history graph

**Steps:**
1. Select a block and let the history accumulate a mix of loud and quiet periods
2. Observe the dashed horizontal line

**Expected:** Dashed line (average of the history window) sits at the mean of the visible polyline. Moves smoothly as the window rolls.

### TC-LM-010: No CPU spike from metering

**Steps:**
1. Note the CPU usage in the header with metering idle
2. Select a block to activate its meter
3. Observe CPU usage while the test tone plays

**Expected:** Enabling measurement on a single block adds a negligible amount of CPU. The K-weighting filter and running-mean computation should not cause noticeable spikes.
