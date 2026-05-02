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
    expect(screen.getAllByText('Mix %').length).toBeGreaterThan(0);
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
    expect(screen.queryAllByText('Mix %').length).toBe(0);
  });

  it('renders Trigger section with BinaryShaping for blockBypass', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="blockBypass"
        blockId="b1"
      />,
    );
    expect(screen.getByText('Trigger')).toBeInTheDocument();
    // State labels follow "OFF · < N" / "ON · ≥ N" format.
    expect(screen.getByText(/OFF\s+·\s+</)).toBeInTheDocument();
    expect(screen.getByText(/ON\s+·\s+≥/)).toBeInTheDocument();
  });

  it('renders Trigger section with BinaryShaping for blockState (IGNORED/RECALL)', () => {
    render(
      <MidiAssignDialog
        open
        onOpenChange={() => {}}
        title="Assign MIDI"
        target="blockState"
        blockId="b1"
        targetIndex={1}
      />,
    );
    expect(screen.getByText('Trigger')).toBeInTheDocument();
    expect(screen.getByText(/IGNORED\s+·\s+</)).toBeInTheDocument();
    expect(screen.getByText(/RECALL\s+·\s+≥/)).toBeInTheDocument();
  });
});
