import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { BinaryShaping } from '../BinaryShaping';
import { TARGET_META } from '../shaping/targetMeta';

describe('BinaryShaping', () => {
  it('renders OFF/ON labels for blockBypass', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={64}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText('OFF')).toBeInTheDocument();
    expect(screen.getByText('ON')).toBeInTheDocument();
  });

  it('renders IGNORED/RECALL labels for blockState', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockState}
        threshold={64}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText('IGNORED')).toBeInTheDocument();
    expect(screen.getByText('RECALL')).toBeInTheDocument();
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

  it('emits onChange when threshold input changes and blurs', () => {
    const onChange = vi.fn();
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={64}
        onChange={onChange}
      />,
    );
    const input = screen.getByRole('spinbutton');
    fireEvent.change(input, { target: { value: '100' } });
    // Live-commit contract: onChange fires on every keystroke that parses
    // to a finite number, not deferred to blur.
    expect(onChange).toHaveBeenCalledWith(100);
    fireEvent.blur(input);
    expect(onChange).toHaveBeenCalledWith(100);
  });
});
