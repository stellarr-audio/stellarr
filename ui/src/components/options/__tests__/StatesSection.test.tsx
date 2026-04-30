import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { useStore } from '../../../store';
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

describe('StatesSection', () => {
  beforeEach(() => {
    useStore.setState({
      midiMappings: [
        { channel: 0, cc: 64, target: 'blockState', blockId: 'block-A', targetIndex: 1 },
      ],
      midiLearning: false,
    });
  });

  const block = {
    id: 'block-A',
    type: 'plugin' as const,
    numStates: 3,
    activeStateIndex: 0,
    dirtyStates: [],
    // Other GridBlock fields are not exercised here.
  } as never;

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
