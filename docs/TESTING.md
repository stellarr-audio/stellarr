# Manual Testing Guide

Automated tests (`make test`) cover graph integrity, audio routing, and state serialisation. This guide covers what they can't: real plugin behaviour, audible quality, and GUI interactions.

You don't need to run every test for every PR -- focus on the sections relevant to your change.

## Setup

- macOS Apple Silicon with at least one VST3 or AU plugin installed
- An audio interface (or built-in output) selected in Options > Audio/MIDI Settings
- A preset directory with 2+ `.stellarr` presets (ideally with different plugin chains)

## Preset switching

This is the most crash-prone area. Test whenever you touch preset loading, graph management, or plugin lifecycle.

1. Load a heavy preset (3+ plugins in the chain)
2. Switch to a different heavy preset -- should not crash
3. Rapidly switch between 3+ presets in quick succession
4. Switch preset while audio is playing through the chain
5. Switch preset while a plugin editor window is open -- editor should close gracefully

**Listen for:**
- Audio gap should be brief (a short click or silence, not seconds of dead air)
- No pops, crackles, or runaway noise after switching
- All plugins in the new preset should produce audio immediately

## Audio device settings

Test whenever you touch `StellarrStandaloneApp.cpp` or audio I/O code.

1. Open Options > Audio/MIDI Settings
2. Change the output device, close the dialog
3. Force-quit the app (Cmd+Q or kill from Activity Monitor)
4. Relaunch -- the changed device should still be selected
5. Toggle "Mute audio input", relaunch -- preference should persist

## Plugin management

Test whenever you touch plugin loading, the Options panel, or block creation.

1. Add a new plugin block, assign a plugin -- should load and process audio
2. Open the plugin's editor window, close it, reopen it
3. Copy a plugin block, paste it -- pasted block should load the same plugin
4. Load a preset that references a plugin you don't have installed -- block should show as missing, not crash
5. Scan for plugins (Settings > Scan) -- UI should remain responsive during scan

## Tuner

Test whenever you touch the tuner, InputBlock, or the Tuner tab.

1. Switch to the Tuner tab -- output should mute
2. Play a note -- tuner should detect pitch within a second
3. Switch between Needle and Strobe modes
4. Change reference pitch (e.g. 432 Hz) -- tuner should show A4 as in-tune at the new frequency
5. Switch back to the Grid tab -- output should unmute, tuner should stop

## Scenes

Test whenever you touch scene management or block state recall.

1. Create 2+ scenes with different plugin states
2. Switch between scenes -- should be instant (no audio gap)
3. Rename and delete a scene
4. Save a preset with scenes, reload it -- scenes should restore correctly

## MIDI

Test whenever you touch MIDI mapping, the MIDI page, or the preset browser.

1. Assign a CC to a parameter (e.g. mix) -- sending that CC should move the parameter
2. Assign Program Change to presets -- sending PC messages should switch presets
3. Assign a CC to scene switching -- sending CC values should recall scenes
4. Use MIDI Learn -- should detect incoming CC automatically
5. Open the MIDI monitor -- incoming messages should appear in real time

## Grid and routing

Test whenever you touch the connection layer, block bypass, or graph topology.

1. Connect blocks in a chain -- audio should flow through
2. Mute a block -- route highlight should break at that block
3. Set a block to Thru bypass -- route highlight should stay connected
4. Drag a block to a new position -- connections should follow
5. Remove a block from the middle of a chain -- connections should clean up

## Edge cases

Worth checking occasionally, especially before a release.

- Launch with no audio device connected
- Launch with a previously-selected audio device unplugged
- Load a corrupt or empty `.stellarr` file
- Create the maximum number of scenes (16) and states (16 per block)
- Fill the grid completely with blocks
