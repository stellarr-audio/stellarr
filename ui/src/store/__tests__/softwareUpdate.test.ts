import { describe, it, expect, beforeEach } from 'vitest';
import { useStore } from '../index';

const initialPayload = {
  status: 'idle' as const,
  latestVersion: '',
  releasedAt: '',
  sizeBytes: 0,
  releaseNotesUrl: '',
  downloadProgress: 0,
  error: '',
};

describe('softwareUpdate slice', () => {
  beforeEach(() => {
    useStore.getState().setSoftwareUpdate(initialPayload);
  });

  it('starts idle', () => {
    expect(useStore.getState().softwareUpdate.status).toBe('idle');
  });

  it('updates on setSoftwareUpdate', () => {
    useStore.getState().setSoftwareUpdate({
      ...initialPayload,
      status: 'available',
      latestVersion: '0.14.0',
      releasedAt: '2026-04-22T00:00:00Z',
      sizeBytes: 19_000_000,
      releaseNotesUrl: 'https://example.com/notes',
    });
    const s = useStore.getState().softwareUpdate;
    expect(s.status).toBe('available');
    expect(s.latestVersion).toBe('0.14.0');
    expect(s.releaseNotesUrl).toBe('https://example.com/notes');
  });
});
