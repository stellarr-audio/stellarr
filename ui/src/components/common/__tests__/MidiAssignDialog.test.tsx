import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, fireEvent, act } from '@testing-library/react';
import { useStore } from '../../../store';
import { MidiAssignDialog } from '../MidiAssignDialog';

vi.mock('../../../bridge', () => ({
  requestStartMidiLearn: vi.fn(),
  requestCancelMidiLearn: vi.fn(),
  requestAddMidiMapping: vi.fn(),
  requestRemoveMidiMapping: vi.fn(),
}));

function resetStore() {
  act(() => {
    useStore.setState({ midiMappings: [], midiLearning: false });
  });
}

describe('MidiAssignDialog', () => {
  beforeEach(resetStore);

  it('renders the Shaping disclosure for continuous targets, collapsed by default', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="blockMix"
        blockId="b1"
      />,
    );
    expect(screen.getByRole('button', { name: /shaping/i })).toBeInTheDocument();
    // Collapsed: anchor inputs not in DOM
    expect(screen.queryByText('Mix value')).not.toBeInTheDocument();
  });

  it('expands the Shaping disclosure on click and shows continuous content', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="blockMix"
        blockId="b1"
      />,
    );
    fireEvent.click(screen.getByRole('button', { name: /shaping/i }));
    expect(screen.getByText('Mix value')).toBeInTheDocument();
  });

  it('hides the Shaping disclosure for kind=none targets', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="presetChange"
      />,
    );
    expect(screen.queryByRole('button', { name: /shaping/i })).not.toBeInTheDocument();
  });
});
