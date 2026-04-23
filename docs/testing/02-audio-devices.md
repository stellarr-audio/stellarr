---
title: Audio Devices
sidebar:
  order: 2
---

Test whenever you touch `StellarrStandaloneApp.cpp`, audio I/O code, or the JUCE `AudioDeviceManager` integration.

## Prerequisites

- At least one audio interface (USB or built-in)
- Ideally a second audio device you can plug/unplug during testing

## Test Cases

### TC-AD-001: Device selection persists across launches

**Steps:**
1. Open Options > Audio/MIDI Settings
2. Change the output device to something other than the default
3. Close the dialog
4. Quit and relaunch the app

**Expected:** The previously selected device is still active.

### TC-AD-002: Device selection persists after crash

**Steps:**
1. Change the audio device in Options
2. Force-quit the app (kill from Activity Monitor, not Cmd+Q)
3. Relaunch

**Expected:** The changed device is still selected. Settings survive the crash.

### TC-AD-003: Mute input preference persists

**Steps:**
1. Open Options > Audio/MIDI Settings
2. Toggle "Mute audio input" on or off
3. Quit and relaunch

**Expected:** The mute preference is remembered.

### TC-AD-004: Launch with unavailable device

**Steps:**
1. Select a USB audio interface as the output device
2. Quit the app
3. Unplug the USB device
4. Relaunch the app

**Expected:** App launches without crash. Falls back to default audio output. Audio works.

### TC-AD-005: Hot-plug audio device

**Steps:**
1. Launch the app with built-in audio
2. Plug in a USB audio interface
3. Open Options > Audio/MIDI Settings
4. Select the newly connected device

**Expected:** Audio routes through the new device. No crash.

### TC-AD-006: Sample rate and buffer size persist

**Steps:**
1. Open Options > Audio/MIDI Settings
2. Change the sample rate (e.g. 44100 to 48000)
3. Change the buffer size
4. Close the dialog, quit, relaunch

**Expected:** Sample rate and buffer size are restored.
