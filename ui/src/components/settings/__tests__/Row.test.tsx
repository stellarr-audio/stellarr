import { render, screen } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import { Row } from '../Row';

describe('Row', () => {
  it('renders info and actions slots', () => {
    render(
      <Row
        info={<span data-testid="info">blurb</span>}
        actions={<button data-testid="action">Click</button>}
      />,
    );
    expect(screen.getByTestId('info')).toBeInTheDocument();
    expect(screen.getByTestId('action')).toBeInTheDocument();
  });

  it('renders without actions', () => {
    render(<Row info={<span>only info</span>} />);
    expect(screen.getByText('only info')).toBeInTheDocument();
  });
});
