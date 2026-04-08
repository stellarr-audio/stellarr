# Settings

Test whenever you touch the Settings page, telemetry, plugin scan, or application preferences.

## Prerequisites

- None beyond a working build

## Test Cases

### TC-SE-001: Plugin scan from Settings

**Steps:**
1. Open the Settings tab
2. Click Scan
3. While scanning, interact with the UI

**Expected:** UI remains responsive. Scan completes. Plugin list updates.

### TC-SE-002: Add and remove scan directories

**Steps:**
1. Open Settings > Scan Directories
2. Add a new directory
3. Remove it

**Expected:** Directories are added and removed. Changes reflect after next scan.

### TC-SE-003: Telemetry toggle

**Steps:**
1. Open Settings
2. Toggle telemetry on
3. Quit and relaunch
4. Check the toggle state

**Expected:** Preference persists across launches.

**Notes:** With no Sentry DSN configured, the toggle still persists the preference but no data is sent.

### TC-SE-004: Telemetry default state

**Steps:**
1. Delete the Stellarr settings file (`~/Library/Application Support/Stellarr.settings`)
2. Launch the app
3. Open Settings

**Expected:** Telemetry is off by default (opt-in model).

### TC-SE-005: Audio/MIDI Settings dialog

**Steps:**
1. Click the Options button in the title bar
2. Choose Audio/MIDI Settings
3. Change a setting (e.g. buffer size)
4. Close the dialog

**Expected:** Setting takes effect immediately. Persists after relaunch (see [Audio Devices](02-audio-devices.md) for full persistence tests).
