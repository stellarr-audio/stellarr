import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import { Numeric } from '../Numeric';

describe('Numeric', () => {
  it('renders children inside a span by default', () => {
    render(<Numeric>440.0 Hz</Numeric>);
    const el = screen.getByText('440.0 Hz');
    expect(el.tagName).toBe('SPAN');
    expect(el.className).not.toBe('');
  });

  it('renders the element given by `as`', () => {
    render(<Numeric as="strong">−12.4 dB</Numeric>);
    expect(screen.getByText('−12.4 dB').tagName).toBe('STRONG');
  });

  it('merges a passed className', () => {
    render(<Numeric className="extra">128</Numeric>);
    expect(screen.getByText('128').className).toContain('extra');
  });

  it('forwards arbitrary props', () => {
    render(<Numeric title="frequency">440</Numeric>);
    expect(screen.getByText('440')).toHaveAttribute('title', 'frequency');
  });
});
