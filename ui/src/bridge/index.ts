import { useStore } from '../store';
import type { GridBlock, Connection, ScanDirectory, PluginInfo } from '../store';

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

function callNativeFunction(name: string, ...args: unknown[]): void {
  const juce = window.__JUCE__;
  if (!juce) return;

  juce.backend.emitEvent('__juce__invoke', {
    name,
    params: args,
    resultId: 0,
  });
}

function extractMessage(detail: unknown): string {
  if (typeof detail === 'object' && detail !== null && 'message' in detail)
    return String((detail as Record<string, unknown>).message);
  return String(detail);
}

function asRecord(detail: unknown): Record<string, unknown> {
  if (typeof detail === 'object' && detail !== null) return detail as Record<string, unknown>;
  return {};
}

// -- Graph commands (UI → C++) -----------------------------------------------

export function requestAddBlock(type: string, col: number, row: number): void {
  sendEvent('addBlock', JSON.stringify({ type, col, row }));
}

export function requestRemoveBlock(blockId: string): void {
  sendEvent('removeBlock', JSON.stringify({ blockId }));
}

export function requestMoveBlock(blockId: string, col: number, row: number): void {
  sendEvent('moveBlock', JSON.stringify({ blockId, col, row }));
}

export function requestAddConnection(sourceId: string, destId: string): void {
  sendEvent('addConnection', JSON.stringify({ sourceId, destId }));
}

export function requestRemoveConnection(sourceId: string, destId: string): void {
  sendEvent('removeConnection', JSON.stringify({ sourceId, destId }));
}

export function requestToggleTestTone(blockId: string): void {
  sendEvent('toggleTestTone', JSON.stringify({ blockId }));
}

export function requestSetBlockPlugin(blockId: string, pluginId: string): void {
  sendEvent('setBlockPlugin', JSON.stringify({ blockId, pluginId }));
}

export function requestOpenPluginEditor(blockId: string): void {
  sendEvent('openPluginEditor', JSON.stringify({ blockId }));
}

export function requestSaveSession(): void {
  sendEvent('saveSession', '');
}

export function requestLoadSession(): void {
  sendEvent('loadSession', '');
}

export function requestPickPresetDirectory(): void {
  sendEvent('pickPresetDirectory', '');
}

export function requestLoadPresetByIndex(index: number): void {
  sendEvent('loadPresetByIndex', JSON.stringify({ index }));
}

export function requestScanPlugins(): void {
  sendEvent('scanPlugins', '');
}

export function requestPickScanDirectory(): void {
  sendEvent('pickScanDirectory', '');
}

export function requestRemoveScanDirectory(path: string): void {
  sendEvent('removeScanDirectory', JSON.stringify({ path }));
}

// -- Core bridge -------------------------------------------------------------

export function sendEvent(eventName: string, payload: string): void {
  if (!bridgeReady) {
    console.warn('[Bridge] Not connected — event not sent:', eventName);
    return;
  }
  callNativeFunction('sendToNative', eventName, payload);
  console.log(`[Bridge] TX ${eventName}:`, payload);
}

export function initBridge(): void {
  const juce = window.__JUCE__;

  if (!juce) {
    console.warn('[Bridge] JUCE backend not available — running outside plugin');
    return;
  }

  console.log('[Bridge] Initialising...');

  // Startup progress
  juce.backend.addEventListener('startupProgress', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setLoadingStatus(
      String(d.status),
      Number(d.progress),
    );
  });

  juce.backend.addEventListener('startupComplete', () => {
    console.log('[Bridge] RX startupComplete');
    useStore.getState().setLoading(false);
  });

  // Welcome / connection
  juce.backend.addEventListener('welcome', (detail: unknown) => {
    console.log('[Bridge] RX welcome:', extractMessage(detail));
    useStore.getState().setConnected(true);
  });

  juce.backend.addEventListener('pong', (detail: unknown) => {
    console.log('[Bridge] RX pong:', extractMessage(detail));
  });

  juce.backend.addEventListener('testToneChanged', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX testToneChanged:', d);
    useStore.getState().setBlockTestTone(String(d.blockId), Boolean(d.enabled));
  });

  // Graph confirmations from C++
  juce.backend.addEventListener('blockAdded', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX blockAdded:', d);
    const blockId = String(d.id);
    useStore.getState().addBlock({
      id: blockId,
      type: String(d.type),
      name: String(d.name),
      col: Number(d.col),
      row: Number(d.row),
      nodeId: Number(d.nodeId),
    });
    useStore.getState().selectBlock(blockId);
  });

  juce.backend.addEventListener('blockRemoved', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX blockRemoved:', d);
    useStore.getState().removeBlock(String(d.blockId));
  });

  juce.backend.addEventListener('blockMoved', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX blockMoved:', d);
    useStore.getState().moveBlock(String(d.blockId), Number(d.col), Number(d.row));
  });

  juce.backend.addEventListener('connectionAdded', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX connectionAdded:', d);
    useStore.getState().addConnection({
      sourceId: String(d.sourceId),
      destId: String(d.destId),
    });
  });

  juce.backend.addEventListener('connectionRemoved', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX connectionRemoved:', d);
    useStore.getState().removeConnection(String(d.sourceId), String(d.destId));
  });

  juce.backend.addEventListener('blockPluginSet', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX blockPluginSet:', d);
    useStore.getState().setBlockPlugin(
      String(d.blockId),
      String(d.pluginId),
      String(d.pluginName),
      Boolean(d.hasEditor),
    );
  });

  juce.backend.addEventListener('graphState', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX graphState');
    const blocks = (Array.isArray(d.blocks) ? d.blocks : []).map(
      (b: unknown) => {
        const r = asRecord(b);
        return {
          id: String(r.id),
          type: String(r.type),
          name: String(r.name),
          col: Number(r.col),
          row: Number(r.row),
          nodeId: Number(r.nodeId),
          pluginId: r.pluginId ? String(r.pluginId) : undefined,
          pluginName: r.pluginName ? String(r.pluginName) : undefined,
        } satisfies GridBlock;
      },
    );
    const connections = (
      Array.isArray(d.connections) ? d.connections : []
    ).map((c: unknown) => {
      const r = asRecord(c);
      return {
        sourceId: String(r.sourceId),
        destId: String(r.destId),
      } satisfies Connection;
    });
    useStore.getState().syncGraph(blocks, connections);
  });

  juce.backend.addEventListener('pluginListUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX pluginListUpdated');
    const plugins = (Array.isArray(d.plugins) ? d.plugins : []).map(
      (p: unknown) => {
        const r = asRecord(p);
        return {
          id: String(r.id),
          name: String(r.name),
          manufacturer: String(r.manufacturer),
          format: String(r.format),
        } satisfies PluginInfo;
      },
    );
    useStore.getState().setAvailablePlugins(plugins);
  });

  juce.backend.addEventListener('scanDirectoriesUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX scanDirectoriesUpdated');
    const dirs = (Array.isArray(d.directories) ? d.directories : []).map(
      (dir: unknown) => {
        const r = asRecord(dir);
        return {
          path: String(r.path),
          isDefault: Boolean(r.isDefault),
        } satisfies ScanDirectory;
      },
    );
    useStore.getState().setScanDirectories(dirs);
  });

  juce.backend.addEventListener('presetListUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX presetListUpdated');
    const files = (Array.isArray(d.files) ? d.files : []).map(String);
    useStore.getState().setPresetList(
      String(d.directory),
      files,
      Number(d.currentIndex),
    );
  });

  juce.backend.addEventListener('systemStats', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setSystemStats(Number(d.cpu), Number(d.memory), Number(d.totalMemory));
  });

  bridgeReady = true;
  callNativeFunction('sendToNative', 'bridgeReady', '');
  console.log('[Bridge] TX bridgeReady');
}
