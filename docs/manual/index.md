---
title: Introduction
description: What Stellarr is, who it's for, and the core ideas.
sidebar:
  order: 1
---

## What is Stellarr?

Stellarr is an open-source signal processing application for musicians. It lets you build custom audio chains using your existing VST3 and Audio Unit plugins, arranged on a visual grid with flexible routing, states, scenes, and MIDI control.

Think of it as a virtual pedalboard and amp rack that you design yourself, using any plugins you already own.

## Philosophy

Stellarr was made by an AI and a human, together. It is free and open, forever. For the love of music and art.

We believe:

- **Musicians should own their signal chain.** Your plugins, your routing, your presets. No vendor lock-in.
- **Simplicity enables creativity.** A clean, focused interface that gets out of your way so you can focus on your sound.
- **Live performance matters.** Scenes, states, and MIDI control are first-class features, not afterthoughts.

## Core Concepts

### Blocks
Everything in Stellarr is a **block**. There are three types:

- **Input** -- receives audio from your audio interface
- **Output** -- sends audio to your speakers or headphones
- **Plugin** -- hosts a VST3 or Audio Unit plugin (amp sim, effect, etc.)

Blocks live on a grid and are connected by wires that carry audio and MIDI.

### States
Each plugin block can save up to **16 states**. A state is a snapshot of:
- The plugin's internal settings (knobs, switches, presets)
- The block's parameters (mix, balance, level, bypass, bypass mode)

Switch between states to instantly recall different configurations of the same plugin.

### Scenes
A **scene** is a preset-level snapshot of which state is active on every block, plus each block's bypass status. Use scenes to switch your entire rig between verse, chorus, solo, or any configuration -- in one click.

### Presets
A **preset** is a complete session file (`.stellarr`) containing:
- All blocks and their positions on the grid
- All connections between blocks
- All states per plugin block
- All scenes
- MIDI CC mappings (preset-level)

### MIDI Control
Map any MIDI CC to any parameter: block bypass, mix, balance, level, scene switching, and more. Use MIDI Learn to auto-detect your controller's CCs, or assign them manually.
