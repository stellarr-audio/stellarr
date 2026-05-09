import { describe, it, expect, beforeEach } from 'vitest';
import { useStore } from '../index';

describe('midi panel slice', () => {
  beforeEach(() => {
    localStorage.removeItem('stellarr.midiPanel.position');
    useStore.getState().setMidiPanelOpen(false);
    useStore.setState({ midiPanelPosition: null });
  });

  it('defaults to closed', () => {
    expect(useStore.getState().midiPanelOpen).toBe(false);
  });

  it('toggleMidiPanel flips the flag', () => {
    useStore.getState().toggleMidiPanel();
    expect(useStore.getState().midiPanelOpen).toBe(true);
    useStore.getState().toggleMidiPanel();
    expect(useStore.getState().midiPanelOpen).toBe(false);
  });

  it('setMidiPanelOpen sets explicit value', () => {
    useStore.getState().setMidiPanelOpen(true);
    expect(useStore.getState().midiPanelOpen).toBe(true);
  });

  it('open/close NOT persisted to localStorage', () => {
    useStore.getState().setMidiPanelOpen(true);
    expect(localStorage.getItem('stellarr.midiPanel.open')).toBeNull();
  });

  it('default panel position is null (use default placement)', () => {
    expect(useStore.getState().midiPanelPosition).toBeNull();
  });

  it('setMidiPanelPosition stores + persists', () => {
    useStore.getState().setMidiPanelPosition({ x: 120, y: 50 });
    expect(useStore.getState().midiPanelPosition).toEqual({ x: 120, y: 50 });
    expect(localStorage.getItem('stellarr.midiPanel.position')).toBe(
      JSON.stringify({ x: 120, y: 50 }),
    );
  });
});
