import { render, screen, act } from '@testing-library/react';
import { describe, it, expect, beforeEach } from 'vitest';
import { SoftwareUpdates } from '../SoftwareUpdates';
import { useStore } from '../../../store';

const resetState = () => {
  act(() => {
    useStore.getState().setSoftwareUpdate({
      status: 'idle',
      latestVersion: '',
      releasedAt: '',
      sizeBytes: 0,
      releaseNotesUrl: '',
      downloadProgress: 0,
      error: '',
    });
  });
};

describe('SoftwareUpdates', () => {
  beforeEach(resetState);

  it('shows the "up to date" line when no update is available', () => {
    act(() => {
      useStore.getState().setSoftwareUpdate({
        status: 'no-update',
        latestVersion: '',
        releasedAt: '',
        sizeBytes: 0,
        releaseNotesUrl: '',
        downloadProgress: 0,
        error: '',
      });
    });
    render(<SoftwareUpdates />);
    expect(screen.getByText(/you're on the latest version/i)).toBeInTheDocument();
    expect(screen.getByRole('button', { name: /install update/i })).toBeDisabled();
  });

  it('shows the update banner when an update is available', () => {
    act(() => {
      useStore.getState().setSoftwareUpdate({
        status: 'available',
        latestVersion: '0.14.0',
        releasedAt: '2026-04-22T00:00:00Z',
        sizeBytes: 19_000_000,
        releaseNotesUrl: 'https://example.com/notes',
        downloadProgress: 0,
        error: '',
      });
    });
    render(<SoftwareUpdates />);
    expect(screen.getByText(/update available/i)).toBeInTheDocument();
    expect(screen.getByText(/v0\.14\.0/)).toBeInTheDocument();
    expect(screen.getByRole('link', { name: /release notes/i })).toBeInTheDocument();
  });

  it('shows "Checking…" button state when checking', () => {
    act(() => {
      useStore.getState().setSoftwareUpdate({
        status: 'checking',
        latestVersion: '',
        releasedAt: '',
        sizeBytes: 0,
        releaseNotesUrl: '',
        downloadProgress: 0,
        error: '',
      });
    });
    render(<SoftwareUpdates />);
    expect(screen.getByRole('button', { name: /checking/i })).toBeDisabled();
  });

  it('shows the error message when status is error', () => {
    act(() => {
      useStore.getState().setSoftwareUpdate({
        status: 'error',
        latestVersion: '',
        releasedAt: '',
        sizeBytes: 0,
        releaseNotesUrl: '',
        downloadProgress: 0,
        error: 'Network unreachable',
      });
    });
    render(<SoftwareUpdates />);
    expect(screen.getByText(/network unreachable/i)).toBeInTheDocument();
  });
});
