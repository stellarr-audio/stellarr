import { describe, it, expect, beforeEach } from 'vitest';
import { useStore, readCellZoom } from '../index';

describe('cell zoom slice', () => {
  beforeEach(() => {
    localStorage.removeItem('stellarr.cellZoom');
    useStore.getState().setCellZoom('M');
  });

  it('defaults to M', () => {
    expect(useStore.getState().cellZoom).toBe('M');
  });

  it('cycleCellZoom(+1) moves M to L', () => {
    useStore.getState().cycleCellZoom(+1);
    expect(useStore.getState().cellZoom).toBe('L');
  });

  it('cycleCellZoom(-1) moves M to S', () => {
    useStore.getState().cycleCellZoom(-1);
    expect(useStore.getState().cellZoom).toBe('S');
  });

  it('cycleCellZoom(+1) clamps at L', () => {
    useStore.getState().setCellZoom('L');
    useStore.getState().cycleCellZoom(+1);
    expect(useStore.getState().cellZoom).toBe('L');
  });

  it('cycleCellZoom(-1) clamps at S', () => {
    useStore.getState().setCellZoom('S');
    useStore.getState().cycleCellZoom(-1);
    expect(useStore.getState().cellZoom).toBe('S');
  });

  it('persists to localStorage', () => {
    useStore.getState().setCellZoom('L');
    expect(localStorage.getItem('stellarr.cellZoom')).toBe('L');
  });

  it('setCellZoom skips persistence on no-op (same value)', () => {
    // beforeEach forces the slice to 'M' which wrote to localStorage; clear it
    // again so we can detect whether the next setCellZoom('M') call writes.
    localStorage.removeItem('stellarr.cellZoom');
    useStore.getState().setCellZoom('M'); // already M — should short-circuit
    expect(localStorage.getItem('stellarr.cellZoom')).toBeNull();
  });
});

describe('readCellZoom', () => {
  beforeEach(() => {
    localStorage.removeItem('stellarr.cellZoom');
  });

  it('returns M when localStorage is empty', () => {
    expect(readCellZoom()).toBe('M');
  });

  it('returns the stored value when valid', () => {
    localStorage.setItem('stellarr.cellZoom', 'S');
    expect(readCellZoom()).toBe('S');
    localStorage.setItem('stellarr.cellZoom', 'L');
    expect(readCellZoom()).toBe('L');
  });

  it('falls back to M when stored value is corrupt', () => {
    localStorage.setItem('stellarr.cellZoom', 'XL');
    expect(readCellZoom()).toBe('M');
  });
});
