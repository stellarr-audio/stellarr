---
title: Manual Testing Guide
sidebar:
  order: 0
---

Automated tests (`make test`) cover graph integrity, audio routing, and state serialisation. This guide covers what they can't: real plugin behaviour, audible quality, GUI interactions, and hardware-dependent flows.

This directory is not part of the hosted user manual. It's an internal reference for contributors and testers.

## When to test

You don't need to run every test case for every PR. Focus on the files relevant to your change:

| If you changed... | Test |
|---|---|
| Preset loading, graph management, plugin lifecycle | [Preset Switching](/docs/testing/01-preset-switching/) |
| `StellarrStandaloneApp.cpp`, audio I/O | [Audio Devices](/docs/testing/02-audio-devices/) |
| Plugin loading, Options panel, block creation | [Plugin Management](/docs/testing/03-plugin-management/) |
| Connection layer, block bypass, graph topology | [Grid and Routing](/docs/testing/04-grid-routing/) |
| Scene management, block state recall | [Scenes](/docs/testing/05-scenes/) |
| Tuner, InputBlock, reference pitch | [Tuner](/docs/testing/06-tuner/) |
| MIDI mapping, MidiMapper, MIDI page | [MIDI](/docs/testing/07-midi/) |
| Settings page, telemetry, persistence | [Settings](/docs/testing/08-settings/) |
| LUFS meter, loudness history, target loudness | [Loudness Metering](/docs/testing/09-loudness-metering/) |
    
## Setup

### Hardware

- macOS Apple Silicon (M1 or later)
- An audio interface or built-in output selected in Options > Audio/MIDI Settings
- Optional: a MIDI controller for MIDI test cases

### Software

- A debug or release build of Stellarr (`make run` or `make run-release`)
- At least 2-3 plugins installed (VST3 or AU format)
- A preset directory with 2+ `.stellarr` presets using different plugin chains

### Finding plugins

Any VST3 or AU plugin will work. If you don't have any installed, these directories are good places to find free (and paid) options:

- [KVR Audio — Best Free Plugins](https://www.kvraudio.com/plugins/best-free-plugins) -- curated lists of top-rated free plugins across all categories
- [Plugins4Free](https://plugins4free.com/) -- large directory of free VST/AU plugins
- [Plugin Boutique — Effects](https://www.pluginboutique.com/categories/2-Effects) -- marketplace with both free and paid plugins

Install 2-3 plugins, scan for them in Stellarr (Settings > Scan), and build a few test presets with different combinations. For stress testing preset switches, use a mix of lightweight and heavy plugins.

## Test case format

Each file uses a consistent format:

```
### TC-XX-NNN: Short descriptive name

**Steps:**
1. First step
2. Second step

**Expected:** What should happen

**Notes:** Context, regression history, or edge case detail
```

The `TC-XX-NNN` identifier (e.g. `TC-PS-001`) is stable and can be referenced in PRs and issues.

## Relationship to automated tests

These test cases intentionally do not duplicate what `make test` already covers. If a manual test case can be automated, it should be -- file an issue or submit a PR to add it to `engine/test/`.
