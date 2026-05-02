import { describe, it, expect } from 'vitest';
import { render, fireEvent } from '@testing-library/react';
import { MappingPreview } from '../MappingPreview';

describe('MappingPreview', () => {
  it('renders anchor numeric labels at ccMin and ccMax inside SVG', () => {
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
    // Anchor labels only — no full-range tick numbers
    expect(texts).toEqual(expect.arrayContaining(['20', '100']));
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

  it('shows hover crosshair and readout when mouse moves inside the plot', () => {
    const { container } = render(
      <MappingPreview
        ccMin={0}
        ccMax={127}
        paramMin={0}
        paramMax={1}
        paramRange={{ min: 0, max: 1 }}
        curve="linear"
        formatParam={(v) => `${Math.round(v * 100)}%`}
      />,
    );
    const svg = container.querySelector('svg')!;
    // Force a known viewport size for the SVG so clientX→viewBox math is predictable.
    const rect = { left: 0, top: 0, width: 320, height: 120, right: 320, bottom: 120, x: 0, y: 0, toJSON: () => ({}) } as DOMRect;
    svg.getBoundingClientRect = () => rect;
    fireEvent.mouseMove(svg, { clientX: 200, clientY: 60 });
    // Hover label is a DOM overlay sibling of the SVG — find it on the container.
    const labels = Array.from(container.querySelectorAll('div')).map((d) => d.textContent ?? '');
    expect(labels.some((t) => t.startsWith('CC ') && t.includes('→'))).toBe(true);
  });

  it('clears hover overlay on mouseLeave', () => {
    const { container } = render(
      <MappingPreview
        ccMin={0}
        ccMax={127}
        paramMin={0}
        paramMax={1}
        paramRange={{ min: 0, max: 1 }}
        curve="linear"
        formatParam={(v) => `${Math.round(v * 100)}%`}
      />,
    );
    const svg = container.querySelector('svg')!;
    const rect = { left: 0, top: 0, width: 320, height: 120, right: 320, bottom: 120, x: 0, y: 0, toJSON: () => ({}) } as DOMRect;
    svg.getBoundingClientRect = () => rect;
    fireEvent.mouseMove(svg, { clientX: 200, clientY: 60 });
    fireEvent.mouseLeave(svg);
    const labels = Array.from(container.querySelectorAll('div')).map((d) => d.textContent ?? '');
    expect(labels.some((t) => t.startsWith('CC ') && t.includes('→'))).toBe(false);
  });
});
