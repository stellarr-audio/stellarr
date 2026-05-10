---
title: Tuner
description: Accurate, always-on pitch detection.
sidebar:
  order: 6
---

Stellarr includes a built-in chromatic tuner for quick tuning between songs or during setup.

## Using the Tuner

1. Click the **Tuner** tab in the navigation bar.
2. Play a note on your instrument.
3. The tuner displays the detected note, octave, frequency, and how far off you are in cents.

## Display

The tuner is designed to be readable from across a stage:

- **Note name** -- Very large text (e.g., **E**, **A**, **G#**). The colour shifts with how in-tune you are:
  - **Green** — within 5 cents of in-tune
  - **Amber** — within 15 cents
  - **Orchid** — more than 15 cents off
- **Octave** -- Shown next to the note name (e.g., **4** for A4 = 440 Hz).
- **Frequency** -- Precise frequency in Hz (e.g., 440.0 Hz).

When no signal is detected, the display shows dashes.

## Needle vs Strobe

The tuner has two visualisation modes, toggled via the tabs inside the Tuner page:

- **Needle** — A horizontal cents bar from −50 to +50. A single needle shows your current offset. Centre = perfectly in tune.
- **Strobe** — A classic strobe band. Vertical stripes scroll left or right; when the note is perfectly in tune they stand still. Faster scroll means further off.

Pick whichever you read better. The mode is remembered across sessions.

## Auto-Mute

When the Tuner tab is active, **audio output is automatically muted**. This lets you tune silently without the audience hearing your open strings. Audio resumes when you switch back to the Grid or any other tab.

## MIDI Toggle

The Tuner can be activated by a MIDI Control Change. Click the **MIDI link icon** next to the Tuner title to open the assign dialog. Pick a CC and threshold (default 64); when an incoming CC value crosses the threshold, the tuner activates AND Stellarr automatically switches to the Tuner tab so the note is on screen the moment your foot hits the switch. CC values below the threshold deactivate the tuner and switch you back to the Grid.

The mapping is global — it persists across preset switches and survives a restart.

## Test Tone

If you don't have an instrument plugged in, you can test the tuner using the built-in test tone (developer mode only — see [System Settings](/docs/system/) for how to enable):

1. Go to the **Grid** tab.
2. Select the Input block.
3. Enable **Test Tone** in the options panel.
4. Switch to the **Tuner** tab.

The tuner will detect the notes from the test melody (a pentatonic riff in E minor).

## Technical Details

The tuner uses the **YIN algorithm** for pitch detection, analysing a 2048-sample buffer at approximately 20 Hz update rate. It is accurate for guitar fundamental frequencies from 30 Hz to 2000 Hz.
