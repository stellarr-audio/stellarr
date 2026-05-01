import { describe, it, expect } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { ShapingDisclosure } from '../ShapingDisclosure';

describe('ShapingDisclosure', () => {
  it('starts collapsed (children hidden)', () => {
    render(
      <ShapingDisclosure>
        <p>shaping content</p>
      </ShapingDisclosure>,
    );
    expect(screen.queryByText('shaping content')).not.toBeInTheDocument();
  });

  it('expands when the header is clicked', () => {
    render(
      <ShapingDisclosure>
        <p>shaping content</p>
      </ShapingDisclosure>,
    );
    fireEvent.click(screen.getByRole('button', { name: /shaping/i }));
    expect(screen.getByText('shaping content')).toBeInTheDocument();
  });
});
