import { describe, it, expect } from 'vitest';
import { render } from '@testing-library/react';
import { MappingPreview } from '../MappingPreview';

describe('MappingPreview', () => {
  it('renders CC tick labels at 0, ccMin, ccMax, 127 inside SVG', () => {
    const { container } = render(
      <MappingPreview
        ccMin={20}
        ccMax={100}
        paramMin={0.3}
        paramMax={0.8}
        paramRange={{ min: 0, max: 1 }}
        curve="linear"
      />,
    );
    const svg = container.querySelector('svg');
    expect(svg).not.toBeNull();
    const texts = Array.from(svg!.querySelectorAll('text')).map((t) => t.textContent);
    expect(texts).toEqual(expect.arrayContaining(['0', '20', '100', '127']));
  });

  it('renders an SVG path for the curve segment', () => {
    const { container } = render(
      <MappingPreview
        ccMin={0}
        ccMax={127}
        paramMin={0}
        paramMax={1}
        paramRange={{ min: 0, max: 1 }}
        curve="linear"
      />,
    );
    expect(container.querySelectorAll('svg path').length).toBeGreaterThan(0);
  });
});
