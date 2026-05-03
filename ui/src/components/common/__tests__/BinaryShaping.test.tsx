import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { BinaryShaping } from '../BinaryShaping';
import { TARGET_META } from '../shaping/targetMeta';

describe('BinaryShaping', () => {
  it('shows help text including the threshold', () => {
    render(
      <BinaryShaping
        meta={TARGET_META.blockBypass}
        threshold={80}
        onChange={vi.fn()}
      />,
    );
    expect(screen.getByText(/Block turns ON when CC ≥ 80/)).toBeInTheDocument();
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
