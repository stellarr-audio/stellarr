import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, act } from '@testing-library/react';
import { useStore } from '../../../store';
import type { GridBlock } from '../../../store';
import { StatesSection } from '../StatesSection';

vi.mock('../../../bridge', () => ({
  requestAddBlockState: vi.fn(),
  requestRecallBlockState: vi.fn(),
  requestDeleteBlockState: vi.fn(),
  requestAddMidiMapping: vi.fn(),
  requestStartMidiLearn: vi.fn(),
  requestRemoveMidiMapping: vi.fn(),
  requestCancelMidiLearn: vi.fn(),
}));

const block: GridBlock = {
  id: 'block-A',
  type: 'plugin',
  name: 'PLG',
  col: 0,
  row: 0,
  nodeId: 100,
  displayName: 'PLG',
  level: 0,
  bypassed: false,
  mix: 1,
  balance: 0,
  numStates: 3,
  activeStateIndex: 0,
  dirtyStates: [],
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

describe('StatesSection', () => {
  beforeEach(() => {
    resetStore();
    act(() => {
      useStore.setState({
        midiMappings: [
          { channel: 0, cc: 64, target: 'blockState', blockId: 'block-A', targetIndex: 1 },
        ],
        midiLearning: false,
      });
    });
  });

  it('shows CC 64 on the state-2 square (targetIndex 1)', () => {
    render(<StatesSection block={block} />);
    expect(screen.getByText('CC 64')).toBeInTheDocument();
  });

  it('opens the MIDI assign dialog when an unassigned state segment is clicked', () => {
    render(<StatesSection block={block} />);
    const buttons = screen.getAllByRole('button', { name: /assign midi to state/i });
    fireEvent.click(buttons[0]); // state 1 (no mapping)
    expect(screen.getByText(/assign midi to state 1/i)).toBeInTheDocument();
  });
});
