import { describe, it, expect, vi } from 'vitest';
import { render, screen, fireEvent } from '@testing-library/react';
import { Tablist, Tab } from '../Tablist';

function Harness({ onChange }: { onChange: (id: string) => void }) {
  return (
    <Tablist value="b" onChange={onChange} aria-label="harness">
      <Tab id="a">A</Tab>
      <Tab id="b">B</Tab>
      <Tab id="c">C</Tab>
    </Tablist>
  );
}

describe('Tablist', () => {
  it('renders tabs with correct roles and aria-selected', () => {
    render(<Harness onChange={() => {}} />);
    const tabs = screen.getAllByRole('tab');
    expect(tabs).toHaveLength(3);
    expect(tabs[1]).toHaveAttribute('aria-selected', 'true');
    expect(tabs[0]).toHaveAttribute('aria-selected', 'false');
  });

  it('calls onChange when a tab is clicked', () => {
    const onChange = vi.fn();
    render(<Harness onChange={onChange} />);
    fireEvent.click(screen.getByText('C'));
    expect(onChange).toHaveBeenCalledWith('c');
  });

  it('arrow keys move selection and wrap', () => {
    const onChange = vi.fn();
    render(<Harness onChange={onChange} />);
    const tabs = screen.getAllByRole('tab');
    tabs[1].focus();
    fireEvent.keyDown(tabs[1], { key: 'ArrowRight' });
    expect(onChange).toHaveBeenLastCalledWith('c');
    fireEvent.keyDown(tabs[1], { key: 'ArrowLeft' });
    expect(onChange).toHaveBeenLastCalledWith('a');
  });

  it('active tab has tabIndex 0, others -1', () => {
    render(<Harness onChange={() => {}} />);
    const tabs = screen.getAllByRole('tab');
    expect(tabs[1]).toHaveAttribute('tabindex', '0');
    expect(tabs[0]).toHaveAttribute('tabindex', '-1');
    expect(tabs[2]).toHaveAttribute('tabindex', '-1');
  });
});
