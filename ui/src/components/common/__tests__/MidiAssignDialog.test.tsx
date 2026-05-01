import { describe, it, expect, vi, beforeEach } from 'vitest';
import { render, screen, act } from '@testing-library/react';
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

  it('renders shaping section for continuous targets always-visible', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="blockMix"
        blockId="b1"
      />,
    );
    // Section header always visible.
    expect(screen.getByText(/shaping/i)).toBeInTheDocument();
    // Continuous content directly visible — no click-to-expand.
    expect(screen.getByText('Mix value')).toBeInTheDocument();
  });

  it('hides the shaping section for kind=none targets', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="presetChange"
      />,
    );
    expect(screen.queryByText('Mix value')).not.toBeInTheDocument();
  });
});
