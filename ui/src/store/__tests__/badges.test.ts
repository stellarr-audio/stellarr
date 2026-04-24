import { describe, it, expect, beforeEach } from 'vitest';
import { useStore } from '../index';

describe('badges slice', () => {
  beforeEach(() => {
    useStore.getState().setBadge('settings', null);
    useStore.getState().setBadge('grid', null);
  });

  it('starts empty', () => {
    expect(useStore.getState().badges.settings).toBeUndefined();
  });

  it('sets a badge by tab id', () => {
    useStore.getState().setBadge('settings', { reason: 'update', severity: 'info' });
    expect(useStore.getState().badges.settings).toEqual({
      reason: 'update',
      severity: 'info',
    });
  });

  it('clears a badge when passed null', () => {
    useStore.getState().setBadge('settings', { reason: 'update', severity: 'info' });
    useStore.getState().setBadge('settings', null);
    expect(useStore.getState().badges.settings).toBeUndefined();
  });

  it('tracks badges per tab independently', () => {
    useStore.getState().setBadge('settings', { reason: 'update', severity: 'info' });
    useStore.getState().setBadge('grid', { reason: 'error', severity: 'danger' });
    expect(useStore.getState().badges.settings?.reason).toBe('update');
    expect(useStore.getState().badges.grid?.severity).toBe('danger');
  });
});
