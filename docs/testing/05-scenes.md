---
title: Scenes
sidebar:
  order: 5
---

Test whenever you touch scene management, block state recall, or the scene dropdown in the preset browser.

## Prerequisites

- A preset with at least one plugin block that has multiple saved states

## Test Cases

### TC-SC-001: Create and recall scenes

**Steps:**
1. Set up a plugin with specific parameters (e.g. gain at 50%)
2. Add a scene (+ Add Scene in the scene dropdown)
3. Change the plugin parameters (e.g. gain at 80%)
4. Add a second scene
5. Recall Scene 1

**Expected:** Plugin parameters revert to the Scene 1 state (gain at 50%). Switch is instant with no audio gap.

### TC-SC-002: Rename a scene

**Steps:**
1. Create a scene
2. Open the scene's sub-menu (dots icon)
3. Choose Rename, enter a new name

**Expected:** Scene name updates in the dropdown.

### TC-SC-003: Delete a scene

**Steps:**
1. Create 2+ scenes
2. Delete one via the sub-menu

**Expected:** Scene is removed. Active scene index adjusts correctly. At least one scene always remains.

### TC-SC-004: Scene bypass state

**Steps:**
1. Bypass a plugin block
2. Save a scene
3. Un-bypass the block
4. Save a second scene
5. Recall the first scene

**Expected:** Block returns to bypassed state.

### TC-SC-005: Scenes persist in preset file

**Steps:**
1. Create 2+ scenes with different states
2. Save the preset
3. Load a different preset
4. Reload the original preset

**Expected:** All scenes are restored with correct names and states.

### TC-SC-006: MIDI scene switching

**Steps:**
1. Assign a CC to scene switching (Link icon on scene dropdown)
2. Send CC values 0, 1, 2 from a MIDI controller

**Expected:** Scenes switch to the corresponding index. No crash.

**Notes:** Requires a MIDI controller or software MIDI source.

### TC-SC-007: Rapid scene switching

**Steps:**
1. Create 3+ scenes
2. Click through them as fast as possible

**Expected:** No crash, no audio artefacts. Final scene state is correct.
