---
title: Plugin Management
sidebar:
  order: 3
---

Test whenever you touch plugin loading, the Options panel, block creation, or the plugin scan system.

## Prerequisites

- 2+ plugins installed (VST3 or AU)
- At least one plugin with a custom editor UI

## Test Cases

### TC-PM-001: Assign plugin to block

**Steps:**
1. Add a new plugin block to the grid
2. Select it, open the Plugin dropdown in the Options panel
3. Choose a plugin

**Expected:** Plugin loads. Audio flows through the block. No delay before audio starts.

### TC-PM-002: Replace plugin on existing block

**Steps:**
1. Assign a plugin to a block (TC-PM-001)
2. Open the Plugin dropdown and select a different plugin

**Expected:** Old plugin is replaced. New plugin produces audio. No crash.

### TC-PM-003: Open and close plugin editor

**Steps:**
1. Assign a plugin with a custom editor
2. Click the gear icon to open the editor window
3. Close the editor window
4. Reopen it

**Expected:** Editor opens and closes without crash. Reopening shows the same plugin state.

### TC-PM-004: Missing plugin on preset load

**Steps:**
1. Save a preset using a plugin
2. Remove or rename that plugin's binary on disk
3. Reload the preset

**Expected:** Block shows as "missing" with the plugin's name. No crash. Other blocks in the chain still work.

### TC-PM-005: Plugin scan responsiveness

**Steps:**
1. Open Settings
2. Click Scan for plugins
3. While scan is running, try interacting with the UI (switch tabs, click around)

**Expected:** UI remains responsive during scan. Scan completes and plugin list updates.

### TC-PM-006: Copy and paste plugin block

**Steps:**
1. Assign a plugin to a block, adjust its parameters
2. Copy the block (right-click or keyboard shortcut)
3. Paste onto an empty cell

**Expected:** Pasted block has the same plugin loaded with the same parameter state.

### TC-PM-007: Add and remove scan directories

**Steps:**
1. Open Settings > Scan Directories
2. Add a new directory containing plugins
3. Scan -- new plugins should appear
4. Remove the directory
5. Scan again -- those plugins should no longer appear in the list

**Expected:** Directories are added/removed correctly. Scan reflects the change.
