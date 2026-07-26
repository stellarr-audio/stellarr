import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, act } from '@testing-library/react';
import { PresetBrowser } from '../PresetBrowser';
import { useStore } from '../../../store';
import * as bridge from '../../../bridge';
import numericStyles from '../../common/Numeric.module.css';

// Seed the store with a known preset so the trigger has a concrete value
// to render and the loading status message can reference it.
function seedPresets() {
  act(() => {
    useStore.setState({
      presetFiles: ['Acme Lead Tone.stellarr', 'Clean Combo.stellarr'],
      currentPresetIndex: 0,
      scenes: [],
      activeSceneIndex: -1,
      blocks: [],
      midiMappings: [],
      isLoadingPreset: false,
    });
  });
}

beforeEach(() => {
  seedPresets();
  vi.restoreAllMocks();
});

// The Preset trigger's accessible name concatenates the "Preset" label and the
// active preset value. Use a permissive matcher anchored on the label.
function getPresetTrigger() {
  return screen.getByRole('button', { name: /preset.*acme lead tone/i });
}

// Radix's DropdownMenu opens on pointerdown (not click) — fireEvent.click
// alone never reaches the trigger's onPointerDown handler in JSDOM.
function openDropdown(trigger: HTMLElement) {
  fireEvent.pointerDown(trigger, { button: 0, pointerId: 1, pointerType: 'mouse' });
  fireEvent.pointerUp(trigger, { button: 0, pointerId: 1, pointerType: 'mouse' });
}

describe('PresetBrowser trigger gating', () => {
  it('renders enabled trigger when isLoadingPreset is false', () => {
    render(<PresetBrowser />);
    const trigger = getPresetTrigger();
    expect(trigger).not.toHaveAttribute('aria-disabled', 'true');
    expect(trigger).not.toHaveAttribute('aria-busy', 'true');
  });

  it('marks trigger aria-disabled and aria-busy when isLoadingPreset is true', () => {
    act(() => {
      useStore.setState({ isLoadingPreset: true });
    });
    render(<PresetBrowser />);
    const trigger = getPresetTrigger();
    expect(trigger).toHaveAttribute('aria-disabled', 'true');
    expect(trigger).toHaveAttribute('aria-busy', 'true');
  });

  it('clicks are no-op while loading', () => {
    act(() => {
      useStore.setState({ isLoadingPreset: true });
    });
    const spy = vi.spyOn(bridge, 'requestLoadPresetByIndex');
    render(<PresetBrowser />);
    const trigger = getPresetTrigger();
    fireEvent.click(trigger);
    expect(spy).not.toHaveBeenCalled();
  });

  it('shows a spinner glyph while loading', () => {
    act(() => {
      useStore.setState({ isLoadingPreset: true });
    });
    render(<PresetBrowser />);
    expect(screen.getByTestId('preset-loading-spinner')).toBeInTheDocument();
  });

  it('exposes an aria-live status while loading', () => {
    act(() => {
      useStore.setState({ isLoadingPreset: true });
    });
    render(<PresetBrowser />);
    const status = screen.getByRole('status');
    expect(status).toHaveTextContent(/loading preset/i);
  });
});

describe('PresetBrowser MIDI CC/PC tag numeric content', () => {
  it('renders the preset PC tag through the Numeric (mono) primitive', () => {
    act(() => {
      useStore.setState({
        midiMappings: [{ channel: -1, cc: -1, target: 'presetChange' }],
      });
    });
    render(<PresetBrowser />);
    openDropdown(screen.getByRole('button', { name: /preset.*acme lead tone/i }));
    const tag = screen.getByText('PC:0');
    expect(tag.className).toContain(numericStyles.numeric);
  });

  it('renders the scene MIDI tag through the Numeric (mono) primitive', () => {
    act(() => {
      useStore.setState({
        scenes: [{ name: 'Scene A', blockStateMap: {} }],
        activeSceneIndex: -1,
        midiMappings: [{ channel: -1, cc: 14, target: 'sceneSwitch' }],
      });
    });
    render(<PresetBrowser />);
    openDropdown(screen.getByRole('button', { name: /scene.*no scene/i }));
    const tag = screen.getByText('CC14 val:0');
    expect(tag.className).toContain(numericStyles.numeric);
  });
});
