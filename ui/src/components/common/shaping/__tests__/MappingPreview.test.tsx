import { describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import { MappingPreview } from '../MappingPreview';

describe('MappingPreview', () => {
  it('renders CC tick labels at 0, ccMin, ccMax, 127', () => {
    render(
      <MappingPreview
        ccMin={20}
        ccMax={100}
        paramMin={0.3}
        paramMax={0.8}
        paramRange={{ min: 0, max: 1 }}
        curve="linear"
      />,
    );
    expect(screen.getByText('CC 0')).toBeInTheDocument();
    expect(screen.getByText('20')).toBeInTheDocument();
    expect(screen.getByText('100')).toBeInTheDocument();
    expect(screen.getByText('127')).toBeInTheDocument();
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
    expect(container.querySelector('svg')).not.toBeNull();
  });
});
