---
title: Tuner
sidebar:
  order: 6
---

Test whenever you touch the tuner, InputBlock pitch detection, reference pitch, tuner modes, or the Tuner tab lifecycle.

## Prerequisites

- An audio input source (guitar, microphone, or test tone)

## Test Cases

### TC-TU-001: Tuner activates on tab switch

**Steps:**
1. Switch to the Tuner tab

**Expected:** Output mutes (no audio from speakers). Tuner display is active and waiting for signal.

### TC-TU-002: Pitch detection

**Steps:**
1. Switch to the Tuner tab
2. Play an A4 note (440 Hz) or enable the test tone

**Expected:** Tuner detects "A" with octave 4. Frequency reads close to 440 Hz. Cents needle is near centre.

### TC-TU-003: Deactivates on tab switch away

**Steps:**
1. Switch to the Tuner tab (output mutes)
2. Switch back to the Grid tab

**Expected:** Audio output resumes. Tuner stops detecting.

### TC-TU-004: Needle mode colour coding

**Steps:**
1. Switch to Tuner tab in Needle mode
2. Play a note that's in tune (< 5 cents off)
3. Play a note that's slightly off (5-15 cents)
4. Play a note that's significantly off (> 15 cents)

**Expected:** Needle is green when in tune, yellow when close, pink when far off.

### TC-TU-005: Strobe mode animation

**Steps:**
1. Switch to Strobe mode in the Tuner sidebar
2. Play an in-tune note
3. Play a sharp note
4. Play a flat note

**Expected:** Stripes are stationary and green when in tune. Stripes scroll right when sharp, left when flat. Speed is proportional to how far off the pitch is.

### TC-TU-006: Reference pitch change

**Steps:**
1. Set reference pitch to 432 Hz in the Tuner sidebar
2. Play an A4 at 432 Hz (or as close as you can)

**Expected:** Tuner shows "A4" as in tune at 432 Hz, not 440 Hz.

### TC-TU-007: Reference pitch persists

**Steps:**
1. Set reference pitch to 442 Hz
2. Quit and relaunch the app
3. Switch to the Tuner tab

**Expected:** Reference pitch is still 442 Hz.

### TC-TU-008: Quick-select pitch buttons

**Steps:**
1. Click each preset button (432, 440, 442, 444)

**Expected:** Reference pitch updates. Active button is highlighted. Number input reflects the value.

### TC-TU-009: No signal state

**Steps:**
1. Switch to Tuner tab with no audio input (or mute your input)

**Expected:** Display shows "--" for note and "-- Hz" for frequency. Needle/strobe is inactive (dim).
