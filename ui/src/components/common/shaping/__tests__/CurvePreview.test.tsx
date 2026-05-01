import { describe, it, expect } from 'vitest';
import { render } from '@testing-library/react';
import { CurvePreview } from '../CurvePreview';

describe('CurvePreview', () => {
  it('renders an SVG path for the chosen curve', () => {
    const { container } = render(<CurvePreview curve="linear" />);
    expect(container.querySelector('svg path')).not.toBeNull();
  });

  it('uses different paths for linear vs log', () => {
    const { container: linear } = render(<CurvePreview curve="linear" />);
    const { container: log } = render(<CurvePreview curve="log" />);
    const linearD = linear.querySelector('svg path')?.getAttribute('d');
    const logD = log.querySelector('svg path')?.getAttribute('d');
    expect(linearD).not.toBeNull();
    expect(logD).not.toBeNull();
    expect(linearD).not.toBe(logD);
  });
});
