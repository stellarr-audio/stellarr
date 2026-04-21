import { describe, it, expect, beforeEach, vi } from 'vitest';
import { render, screen, fireEvent, act } from '@testing-library/react';
import { useStore } from '../../../store';
import type { GridBlock } from '../../../store';
import { OptionsPanel } from '../OptionsPanel';

const basePluginBlock: GridBlock = {
  id: 'blk-plugin-1',
  type: 'plugin',
  name: 'PLG',
  col: 0,
  row: 0,
  nodeId: 100,
  displayName: 'RVB',
  level: 0,
  bypassed: false,
  mix: 1,
  balance: 0,
  numStates: 16,
  activeStateIndex: 0,
  dirtyStates: [],
  pluginName: 'Test Plugin',
  pluginFormat: 'VST3',
};

const baseInputBlock: GridBlock = {
  id: 'blk-input-1',
  type: 'input',
  name: 'IN',
  col: 0,
  row: 0,
  nodeId: 1,
  displayName: 'IN',
  level: 0,
};

function resetStore() {
  act(() => {
    useStore.setState({
      blocks: [],
      selectedBlockId: null,
      floatingPanelPos: null,
      midiMappings: [],
      availablePlugins: [],
      lufsByBlockId: {},
      targetLufsByBlockId: {},
      loudnessHistory: [],
      testToneSamples: [],
    });
  });
}

describe('OptionsPanel', () => {
  beforeEach(() => {
    resetStore();
  });

  it('renders nothing when no block selected', () => {
    const { container } = render(<OptionsPanel />);
    expect(container.firstChild).toBeNull();
  });

  it('renders the panel with block display name when a plugin block is selected', () => {
    act(() => {
      useStore.setState({ blocks: [basePluginBlock], selectedBlockId: basePluginBlock.id });
    });
    render(<OptionsPanel />);
    expect(screen.getByText('RVB')).toBeInTheDocument();
  });

  it('shows bypass toggle + MIDI badge for plugin blocks', () => {
    act(() => {
      useStore.setState({ blocks: [basePluginBlock], selectedBlockId: basePluginBlock.id });
    });
    render(<OptionsPanel />);
    expect(screen.getByRole('button', { name: /Assign MIDI CC to bypass/i })).toBeInTheDocument();
  });

  it('hides bypass + MIDI for input blocks', () => {
    act(() => {
      useStore.setState({ blocks: [baseInputBlock], selectedBlockId: baseInputBlock.id });
    });
    render(<OptionsPanel />);
    expect(screen.queryByRole('button', { name: /Assign MIDI CC to bypass/i })).toBeNull();
  });

  it('close button clears the selection', () => {
    act(() => {
      useStore.setState({ blocks: [basePluginBlock], selectedBlockId: basePluginBlock.id });
    });
    const { rerender } = render(<OptionsPanel />);
    fireEvent.click(screen.getByRole('button', { name: /Close panel/i }));
    rerender(<OptionsPanel />);
    expect(useStore.getState().selectedBlockId).toBeNull();
  });

  it('Esc key clears the selection', () => {
    act(() => {
      useStore.setState({ blocks: [basePluginBlock], selectedBlockId: basePluginBlock.id });
    });
    render(<OptionsPanel />);
    fireEvent.keyDown(window, { key: 'Escape' });
    expect(useStore.getState().selectedBlockId).toBeNull();
  });

  it('deselecting a block resets the floating panel position to null', () => {
    act(() => {
      useStore.setState({
        blocks: [basePluginBlock],
        selectedBlockId: basePluginBlock.id,
        floatingPanelPos: { x: 120, y: 80 },
      });
    });
    act(() => {
      useStore.getState().selectBlock(null);
    });
    expect(useStore.getState().floatingPanelPos).toBeNull();
  });

  it('uses stored position when provided', () => {
    act(() => {
      useStore.setState({
        blocks: [basePluginBlock],
        selectedBlockId: basePluginBlock.id,
        floatingPanelPos: { x: 42, y: 73 },
      });
    });
    const { container } = render(<OptionsPanel />);
    const panel = container.querySelector('[class*="panel"]') as HTMLElement | null;
    expect(panel).not.toBeNull();
    expect(panel?.style.left).toBe('42px');
    expect(panel?.style.top).toBe('73px');
  });

  it('defaults to a top-right corner when no stored position exists', () => {
    act(() => {
      useStore.setState({ blocks: [basePluginBlock], selectedBlockId: basePluginBlock.id });
    });
    const { container } = render(<OptionsPanel />);
    const panel = container.querySelector('[class*="panel"]') as HTMLElement | null;
    expect(panel?.style.right).toBe('16px');
    expect(panel?.style.top).toBe('16px');
  });

  it('displays an assigned MIDI CC label when a bypass mapping exists', () => {
    vi.useFakeTimers();
    act(() => {
      useStore.setState({
        blocks: [basePluginBlock],
        selectedBlockId: basePluginBlock.id,
        midiMappings: [{ channel: 0, cc: 42, target: 'blockBypass', blockId: basePluginBlock.id }],
      });
    });
    render(<OptionsPanel />);
    expect(screen.getByRole('button', { name: /Bypass MIDI: CC 42/i })).toBeInTheDocument();
    vi.useRealTimers();
  });
});
