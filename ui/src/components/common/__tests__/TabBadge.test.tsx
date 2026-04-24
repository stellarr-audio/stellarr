import { render } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import { TabBadge } from '../TabBadge';

describe('TabBadge', () => {
  it('renders nothing when badge is undefined', () => {
    const { container } = render(<TabBadge badge={undefined} />);
    expect(container.firstChild).toBeNull();
  });

  it('renders a dot when badge is present', () => {
    const { container } = render(
      <TabBadge badge={{ reason: 'update', severity: 'info' }} />,
    );
    expect(container.querySelector('[data-severity="info"]')).toBeTruthy();
  });

  it('reflects severity as a data attribute', () => {
    const { container } = render(
      <TabBadge badge={{ reason: 'error', severity: 'danger' }} />,
    );
    expect(container.querySelector('[data-severity="danger"]')).toBeTruthy();
  });
});
