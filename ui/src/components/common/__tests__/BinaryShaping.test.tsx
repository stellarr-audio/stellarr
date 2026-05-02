import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { BinaryShaping } from '../BinaryShaping';
import { TARGET_META } from '../shaping/targetMeta';

describe('BinaryShaping', () => {
  it('renders OFF and ON state labels for blockBypass', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={64}
        onChange={vi.fn()}
      />,
    );
    // State labels follow "OFF · < N" / "ON · ≥ N" format.
    expect(screen.getByText(/OFF\s+·\s+<\s+64/)).toBeInTheDocument();
    expect(screen.getByText(/ON\s+·\s+≥\s+64/)).toBeInTheDocument();
  });

  it('renders IGNORED and RECALL state labels for blockState', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockState}
        threshold={64}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText(/IGNORED\s+·\s+<\s+64/)).toBeInTheDocument();
    expect(screen.getByText(/RECALL\s+·\s+≥\s+64/)).toBeInTheDocument();
  });

  it('shows help text including the threshold', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={80}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText(/CC ≥ 80/)).toBeInTheDocument();
  });

  it('emits onChange when slider value changes', () => {
    const onChange = vi.fn();
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={64}
        onChange={onChange}
      />,
    );
    const slider = screen.getByRole('slider');
    fireEvent.change(slider, { target: { value: '100' } });
    expect(onChange).toHaveBeenCalledWith(100);
  });
});
