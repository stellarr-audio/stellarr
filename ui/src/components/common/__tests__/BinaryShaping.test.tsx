import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { BinaryShaping } from '../BinaryShaping';
import { TARGET_META } from '../shaping/targetMeta';

describe('BinaryShaping', () => {
  it('shows the rule line including the threshold', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={80}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText(/ON ≥ 80/)).toBeInTheDocument();
  });

  it('uses RECALL label for blockState', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockState}
        threshold={80}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText(/RECALL ≥ 80/)).toBeInTheDocument();
  });

  it('renders a Threshold field label', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={80}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText('Threshold')).toBeInTheDocument();
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
