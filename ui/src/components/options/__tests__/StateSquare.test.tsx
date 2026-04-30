import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { StateSquare } from '../StateSquare';

describe('StateSquare', () => {
  const baseProps = {
    index: 0,
    isActive: false,
    isDirty: false,
    canDelete: true,
    onRecall: vi.fn(),
    onDelete: vi.fn(),
    onAssignMidi: vi.fn(),
  };

  it('renders the link icon button when no MIDI mapping is assigned', () => {
    render(<StateSquare {...baseProps} midiMapping={undefined} />);
    expect(screen.getByRole('button', { name: /assign midi/i })).toBeInTheDocument();
    expect(screen.queryByText(/^CC \d+/i)).not.toBeInTheDocument();
  });

  it('renders the CC NN segment when a CC mapping is assigned', () => {
    render(
      <StateSquare
        {...baseProps}
        midiMapping={{ channel: 0, cc: 64, target: 'blockState', blockId: 'b1', targetIndex: 0 }}
      />,
    );
    expect(screen.getByText('CC 64')).toBeInTheDocument();
  });

  it('opens the assign dialog on MIDI segment click', () => {
    const onAssignMidi = vi.fn();
    render(<StateSquare {...baseProps} onAssignMidi={onAssignMidi} midiMapping={undefined} />);
    fireEvent.click(screen.getByRole('button', { name: /assign midi/i }));
    expect(onAssignMidi).toHaveBeenCalledTimes(1);
  });
});
