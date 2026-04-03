# Tuner

Stellarr includes a built-in chromatic tuner for quick tuning between songs or during setup.

## Using the Tuner

1. Click the **Tuner** tab in the navigation bar.
2. Play a note on your instrument.
3. The tuner displays the detected note, octave, frequency, and how far off you are in cents.

## Display

The tuner is designed to be readable from across a stage:

- **Note name** -- Very large text (e.g., **E**, **A**, **G#**). Colour-coded:
  - **Green** -- within 5 cents of in-tune
  - **Yellow** -- within 15 cents
  - **Pink** -- more than 15 cents off
- **Octave** -- Shown next to the note name (e.g., **4** for A4 = 440 Hz).
- **Frequency** -- Precise frequency in Hz (e.g., 440.0 Hz).
- **Cents bar** -- A horizontal gauge from -50 to +50 cents. A needle shows your current offset. Centre = perfectly in tune.

When no signal is detected, the display shows dashes.

## Auto-Mute

When the Tuner tab is active, **audio output is automatically muted**. This lets you tune silently without the audience hearing your open strings. Audio resumes when you switch back to the Grid or any other tab.

## Test Tone

If you don't have an instrument plugged in, you can test the tuner using the built-in test tone:

1. Go to the **Grid** tab.
2. Select the Input block.
3. Enable **Test Tone** in the options panel.
4. Switch to the **Tuner** tab.

The tuner will detect the notes from the test melody (a pentatonic riff in E minor).

## Technical Details

The tuner uses the **YIN algorithm** for pitch detection, analysing a 2048-sample buffer at approximately 20 Hz update rate. It is accurate for guitar fundamental frequencies from 30 Hz to 2000 Hz.
