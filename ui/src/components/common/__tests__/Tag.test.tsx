import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { Tag } from '../Tag';

describe('Tag', () => {
  it('renders children and calls onClick', () => {
    const onClick = vi.fn();
    render(<Tag onClick={onClick}>440</Tag>);
    const btn = screen.getByRole('button', { name: '440' });
    fireEvent.click(btn);
    expect(onClick).toHaveBeenCalledTimes(1);
  });

  it('applies active class when active prop is true', () => {
    const { container } = render(<Tag active>440</Tag>);
    const btn = container.querySelector('button');
    expect(btn?.className).toMatch(/active/);
  });

  it('is a button type="button" by default', () => {
    render(<Tag>x</Tag>);
    expect(screen.getByRole('button')).toHaveAttribute('type', 'button');
  });
});
