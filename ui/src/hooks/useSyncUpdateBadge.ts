import { useEffect } from 'react';
import { useStore } from '../store';

// Keep the tab-bar badge on the System tab in lockstep with the update
// state: visible whenever an update is available or has finished
// downloading and is waiting to install.
export function useSyncUpdateBadge() {
  const status = useStore((s) => s.softwareUpdate.status);
  const setBadge = useStore((s) => s.setBadge);

  useEffect(() => {
    if (status === 'available' || status === 'ready') {
      setBadge('settings', { reason: 'update', severity: 'info' });
    } else {
      setBadge('settings', null);
    }
  }, [status, setBadge]);
}
