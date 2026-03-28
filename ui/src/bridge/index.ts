import { useStore } from '../store';

declare global {
  interface Window {
    __JUCE__?: {
      backend: {
        addEventListener: (
          eventId: string,
          callback: (payload: unknown) => void,
        ) => [string, number];
        emitEvent: (eventId: string, payload: unknown) => void;
      };
      initialisationData: {
        __juce__functions: string[];
      };
    };
  }
}

let bridgeReady = false;

function extractMessage(detail: unknown): string {
  if (
    typeof detail === 'object' &&
    detail !== null &&
    'message' in detail
  )
    return String((detail as Record<string, unknown>).message);
  return String(detail);
}

function callNativeFunction(name: string, ...args: unknown[]): void {
  const juce = window.__JUCE__;
  if (!juce) return;

  juce.backend.emitEvent('__juce__invoke', {
    name,
    params: args,
    resultId: 0,
  });
}

export function initBridge(): void {
  const juce = window.__JUCE__;

  if (!juce) {
    console.warn('[Bridge] JUCE backend not available — running outside plugin');
    return;
  }

  console.log('[Bridge] Initialising...');

  juce.backend.addEventListener('welcome', (detail: unknown) => {
    const message = extractMessage(detail);
    console.log('[Bridge] RX welcome:', message);
    useStore.getState().setConnected(true);
  });

  juce.backend.addEventListener('pong', (detail: unknown) => {
    const message = extractMessage(detail);
    console.log('[Bridge] RX pong:', message);
  });

  bridgeReady = true;
  callNativeFunction('sendToNative', 'bridgeReady', '');
  console.log('[Bridge] TX bridgeReady');
}

export function sendEvent(eventName: string, payload: string): void {
  if (!bridgeReady) {
    console.warn('[Bridge] Not connected — event not sent:', eventName);
    return;
  }

  callNativeFunction('sendToNative', eventName, payload);
  console.log(`[Bridge] TX ${eventName}:`, payload);
}
