import * as Sentry from '@sentry/browser';
import { useStore } from '../store';
import type {
  GridBlock,
  Connection,
  ScanDirectory,
  PluginInfo,
  Scene,
  MidiMapping,
  MidiMonitorEvent,
} from '../store';

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

export function requestAddBlock(
  type: string,
  col: number,
  row: number,
  spliceSourceId?: string,
  spliceDestId?: string,
): void {
  sendEvent(
    'addBlock',
    JSON.stringify({
      type,
      col,
      row,
      ...(spliceSourceId && spliceDestId ? { spliceSourceId, spliceDestId } : {}),
    }),
  );
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

export function requestSetBlockMix(blockId: string, mix: number): void {
  sendEvent('setBlockMix', JSON.stringify({ blockId, mix }));
}

export function requestSetBlockLevel(blockId: string, level: number): void {
  sendEvent('setBlockLevel', JSON.stringify({ blockId, level }));
}

export function requestSetBlockBalance(blockId: string, balance: number): void {
  sendEvent('setBlockBalance', JSON.stringify({ blockId, balance }));
}

export function requestToggleBlockBypass(blockId: string): void {
  sendEvent('toggleBlockBypass', JSON.stringify({ blockId }));
}

export function requestSetBlockBypassMode(blockId: string, bypassMode: string): void {
  sendEvent('setBlockBypassMode', JSON.stringify({ blockId, bypassMode }));
}

export function requestGetTestToneSamples(): void {
  sendEvent('getTestToneSamples', '');
}

export function requestSetTestToneSample(blockId: string, sample: string): void {
  sendEvent('setTestToneSample', JSON.stringify({ blockId, sample }));
}

export function requestToggleTestTone(blockId: string): void {
  sendEvent('toggleTestTone', JSON.stringify({ blockId }));
}

export function requestSetBlockColor(blockId: string, color: string): void {
  sendEvent('setBlockColor', JSON.stringify({ blockId, color }));
}

export function requestRenameBlock(blockId: string, name: string): void {
  sendEvent('renameBlock', JSON.stringify({ blockId, name }));
}

export function requestCopyBlock(blockId: string): void {
  sendEvent('copyBlock', JSON.stringify({ blockId }));
}

export function requestPasteBlock(col: number, row: number): void {
  sendEvent('pasteBlock', JSON.stringify({ col, row }));
}

export function requestSetBlockPlugin(blockId: string, pluginId: string): void {
  sendEvent('setBlockPlugin', JSON.stringify({ blockId, pluginId }));
}

export function requestOpenPluginEditor(blockId: string): void {
  sendEvent('openPluginEditor', JSON.stringify({ blockId }));
}

export function requestNewSession(): void {
  sendEvent('newSession', '');
}

export function requestSaveSession(): void {
  sendEvent('saveSession', '');
}

export function requestSaveSessionQuiet(): void {
  sendEvent('saveSessionQuiet', '');
}

export function requestSetGridSize(columns: number, rows: number): void {
  sendEvent('setGridSize', JSON.stringify({ columns, rows }));
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

export function requestRenamePreset(index: number, name: string): void {
  sendEvent('renamePreset', JSON.stringify({ index, name }));
}

export function requestDeletePreset(index: number): void {
  sendEvent('deletePreset', JSON.stringify({ index }));
}

export function requestSaveBlockState(blockId: string): void {
  sendEvent('saveBlockState', JSON.stringify({ blockId }));
}

export function requestAddBlockState(blockId: string): void {
  sendEvent('addBlockState', JSON.stringify({ blockId }));
}

export function requestRecallBlockState(blockId: string, index: number): void {
  sendEvent('recallBlockState', JSON.stringify({ blockId, index }));
}

export function requestDeleteBlockState(blockId: string, index: number): void {
  sendEvent('deleteBlockState', JSON.stringify({ blockId, index }));
}

// -- Scene commands -----------------------------------------------------------

export function requestAddScene(): void {
  sendEvent('addScene', '{}');
}

export function requestRecallScene(index: number): void {
  sendEvent('recallScene', JSON.stringify({ index }));
}

export function requestSaveScene(index: number): void {
  sendEvent('saveScene', JSON.stringify({ index }));
}

export function requestRenameScene(index: number, name: string): void {
  sendEvent('renameScene', JSON.stringify({ index, name }));
}

export function requestDeleteScene(index: number): void {
  sendEvent('deleteScene', JSON.stringify({ index }));
}

// -- MIDI mapping commands ----------------------------------------------------

export function requestAddMidiMapping(
  channel: number,
  cc: number,
  target: string,
  blockId?: string,
): void {
  sendEvent('addMidiMapping', JSON.stringify({ channel, cc, target, blockId: blockId ?? '' }));
}

export function requestRemoveMidiMapping(index: number): void {
  sendEvent('removeMidiMapping', JSON.stringify({ index }));
}

export function requestClearMidiMappings(): void {
  sendEvent('clearMidiMappings', '');
}

export function requestGetMidiMappings(): void {
  sendEvent('getMidiMappings', '');
}

export function requestStartMidiLearn(target: string, blockId?: string): void {
  sendEvent('startMidiLearn', JSON.stringify({ target, blockId: blockId ?? '' }));
}

export function requestCancelMidiLearn(): void {
  sendEvent('cancelMidiLearn', '');
}

export function requestSetMidiMonitorEnabled(enabled: boolean): void {
  sendEvent('setMidiMonitorEnabled', JSON.stringify({ enabled }));
}

export function requestInjectMidiCC(channel: number, cc: number, value: number): void {
  sendEvent('injectMidiCC', JSON.stringify({ channel, cc, value }));
}

export function requestSetTunerEnabled(enabled: boolean): void {
  sendEvent('setTunerEnabled', JSON.stringify({ enabled }));
}

export function requestGetReferencePitch(): void {
  sendEvent('getReferencePitch', '');
}

export function requestSetReferencePitch(hz: number): void {
  sendEvent('setReferencePitch', JSON.stringify({ hz }));
}

export function requestScanPlugins(): void {
  sendEvent('scanPlugins', '');
}

export function requestGetTelemetryEnabled(): void {
  sendEvent('getTelemetryEnabled', '');
}

export function requestSetTelemetryEnabled(enabled: boolean): void {
  sendEvent('setTelemetryEnabled', JSON.stringify({ enabled }));
}

export function requestPickScanDirectory(): void {
  sendEvent('pickScanDirectory', '');
}

export function requestRemoveScanDirectory(path: string): void {
  sendEvent('removeScanDirectory', JSON.stringify({ path }));
}

// -- Loudness metering commands -----------------------------------------------

export function requestSetSelectedBlock(blockId: string): void {
  sendEvent('setSelectedBlock', JSON.stringify({ blockId }));
}

export function requestSetTargetLufs(blockId: string, lufs: number | null): void {
  sendEvent('setTargetLufs', JSON.stringify({ blockId, lufs }));
}

export function requestSetLufsWindow(window: 'momentary' | 'shortTerm'): void {
  sendEvent('setLufsWindow', JSON.stringify({ window }));
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
    useStore.getState().setLoadingStatus(String(d.status), Number(d.progress));
  });

  juce.backend.addEventListener('startupComplete', () => {
    console.log('[Bridge] RX startupComplete');
    useStore.getState().setLoading(false);
    // Signal native to show WebView after React paints
    requestAnimationFrame(() => sendEvent('uiReady', ''));
  });

  // Screenshot automation
  juce.backend.addEventListener('screenshotSetup', (detail: unknown) => {
    const d = asRecord(detail);

    // Apply theme before navigation so first paint is in the correct mode.
    const theme = d.theme;
    if (theme === 'light' || theme === 'dark' || theme === 'system') {
      import('../store/theme').then(({ useThemeStore }) => {
        useThemeStore.getState().setTheme(theme);
      });
    }

    // Navigate to page
    const page = String(d.page || 'grid');
    useStore.getState().setActiveTab(page);

    // Delay actions to allow graph state to arrive and render
    setTimeout(() => {
      const store = useStore.getState();
      const actions = d.actions as Array<Record<string, unknown>> | undefined;
      if (Array.isArray(actions)) {
        for (const action of actions) {
          if (action.selectBlock) {
            const val = action.selectBlock;
            if (typeof val === 'string') {
              // Select by block ID
              const block = store.blocks.find((b) => b.id === val);
              if (block) store.selectBlock(block.id);
            } else {
              // Select by grid position (legacy)
              const pos = val as Record<string, unknown>;
              const col = Number(pos.col);
              const row = Number(pos.row);
              const block = store.blocks.find((b) => b.col === col && b.row === row);
              if (block) store.selectBlock(block.id);
            }
          } else if (action.setTestToneSample) {
            const val = action.setTestToneSample as Record<string, unknown>;
            const blockId = String(val.blockId ?? '');
            const sample = String(val.sample ?? '');
            if (blockId) requestSetTestToneSample(blockId, sample);
          } else if (action.toggleTestTone) {
            const blockId = String(action.toggleTestTone);
            if (blockId) requestToggleTestTone(blockId);
          }
        }
      }

      // Wait for render then signal ready
      setTimeout(() => sendEvent('screenshotReady', ''), 500);
    }, 1000);
  });

  // Welcome / connection
  juce.backend.addEventListener('welcome', (detail: unknown) => {
    console.log('[Bridge] RX welcome:', extractMessage(detail));
    useStore.getState().setConnected(true);
  });

  juce.backend.addEventListener('pong', (detail: unknown) => {
    console.log('[Bridge] RX pong:', extractMessage(detail));
  });

  juce.backend.addEventListener('testToneSamplesUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    const samples = Array.isArray(d.samples) ? (d.samples as unknown[]).map(String) : [];
    useStore.getState().setTestToneSamples(samples);
  });

  juce.backend.addEventListener('testToneSampleChanged', (detail: unknown) => {
    const d = asRecord(detail);
    const blockId = String(d.blockId);
    const sample = String(d.sample);
    useStore.getState().setTestToneSample(sample);
    // Store per-block
    useStore.setState((s) => ({
      blocks: s.blocks.map((b) => (b.id === blockId ? { ...b, testToneSample: sample } : b)),
    }));
  });

  juce.backend.addEventListener('testToneChanged', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX testToneChanged:', d);
    useStore.getState().setBlockTestTone(String(d.blockId), Boolean(d.enabled));
  });

  juce.backend.addEventListener('blockMixChanged', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockMix(String(d.blockId), Number(d.mix));
  });

  juce.backend.addEventListener('blockLevelChanged', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockLevel(String(d.blockId), Number(d.level));
  });

  juce.backend.addEventListener('blockBalanceChanged', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockBalance(String(d.blockId), Number(d.balance));
  });

  juce.backend.addEventListener('blockBypassChanged', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockBypassed(String(d.blockId), Boolean(d.bypassed));
  });

  juce.backend.addEventListener('blockBypassModeChanged', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockBypassMode(String(d.blockId), String(d.bypassMode));
  });

  // Graph confirmations from C++
  juce.backend.addEventListener('blockCopied', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setClipboardBlockType(String(d.type));
  });

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

  juce.backend.addEventListener('blockColorChanged', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockColor(String(d.blockId), String(d.blockColor));
  });

  juce.backend.addEventListener('blockRenamed', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setBlockDisplayName(String(d.blockId), String(d.displayName));
  });

  juce.backend.addEventListener('blockPluginSet', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX blockPluginSet:', d);
    useStore
      .getState()
      .setBlockPlugin(
        String(d.blockId),
        String(d.pluginId),
        String(d.pluginName),
        String(d.pluginFormat),
        Boolean(d.hasEditor),
      );
  });

  juce.backend.addEventListener('graphState', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX graphState');
    const blocks = (Array.isArray(d.blocks) ? d.blocks : []).map((b: unknown) => {
      const r = asRecord(b);
      return {
        id: String(r.id),
        type: String(r.type),
        name: String(r.name),
        col: Number(r.col),
        row: Number(r.row),
        nodeId: Number(r.nodeId),
        displayName: r.displayName ? String(r.displayName) : undefined,
        blockColor: r.blockColor ? String(r.blockColor) : undefined,
        pluginId: r.pluginId ? String(r.pluginId) : undefined,
        pluginName: r.pluginName ? String(r.pluginName) : undefined,
        pluginFormat: r.pluginFormat ? String(r.pluginFormat) : undefined,
        pluginMissing: r.pluginMissing ? Boolean(r.pluginMissing) : undefined,
        mix: r.mix !== undefined ? Number(r.mix) : undefined,
        balance: r.balance !== undefined ? Number(r.balance) : undefined,
        level: r.level !== undefined ? Number(r.level) : undefined,
        bypassed: r.bypassed ? Boolean(r.bypassed) : undefined,
        bypassMode: r.bypassMode ? String(r.bypassMode) : undefined,
        numStates: r.numStates !== undefined ? Number(r.numStates) : undefined,
        activeStateIndex: r.activeStateIndex !== undefined ? Number(r.activeStateIndex) : undefined,
        dirtyStates: Array.isArray(r.dirtyStates)
          ? (r.dirtyStates as unknown[]).map(Number)
          : undefined,
      } satisfies GridBlock;
    });
    const connections = (Array.isArray(d.connections) ? d.connections : []).map((c: unknown) => {
      const r = asRecord(c);
      return {
        sourceId: String(r.sourceId),
        destId: String(r.destId),
      } satisfies Connection;
    });
    useStore.getState().syncGraph(blocks, connections);
  });

  juce.backend.addEventListener('gridState', (detail: unknown) => {
    const d = asRecord(detail);
    const columns = Number(d.columns);
    const rows = Number(d.rows);
    if (Number.isFinite(columns) && Number.isFinite(rows)) {
      useStore.getState().setGridSize(columns, rows);
    }
  });

  juce.backend.addEventListener('scanStarted', () => {
    useStore.getState().setScanning(true);
  });

  juce.backend.addEventListener('pluginListUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX pluginListUpdated');
    const plugins = (Array.isArray(d.plugins) ? d.plugins : []).map((p: unknown) => {
      const r = asRecord(p);
      return {
        id: String(r.id),
        name: String(r.name),
        manufacturer: String(r.manufacturer),
        format: String(r.format),
      } satisfies PluginInfo;
    });
    useStore.getState().setAvailablePlugins(plugins);
  });

  juce.backend.addEventListener('scanDirectoriesUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX scanDirectoriesUpdated');
    const dirs = (Array.isArray(d.directories) ? d.directories : []).map((dir: unknown) => {
      const r = asRecord(dir);
      return {
        path: String(r.path),
        isDefault: Boolean(r.isDefault),
      } satisfies ScanDirectory;
    });
    useStore.getState().setScanDirectories(dirs);
  });

  juce.backend.addEventListener('presetListUpdated', (detail: unknown) => {
    const d = asRecord(detail);
    console.log('[Bridge] RX presetListUpdated');
    const files = (Array.isArray(d.files) ? d.files : []).map(String);
    useStore.getState().setPresetList(String(d.directory), files, Number(d.currentIndex));
  });

  juce.backend.addEventListener('blockStatesChanged', (detail: unknown) => {
    const d = asRecord(detail);
    const dirty = Array.isArray(d.dirtyStates) ? (d.dirtyStates as unknown[]).map(Number) : [];
    useStore
      .getState()
      .setBlockStates(String(d.blockId), Number(d.numStates), Number(d.activeStateIndex), dirty);
  });

  juce.backend.addEventListener('scenesChanged', (detail: unknown) => {
    const d = asRecord(detail);
    const scenes = (Array.isArray(d.scenes) ? d.scenes : []).map((s: unknown) => {
      const r = asRecord(s);
      const mapRaw = asRecord(r.blockStateMap);
      const blockStateMap: Record<string, number> = {};
      for (const [k, v] of Object.entries(mapRaw)) blockStateMap[k] = Number(v);
      return { name: String(r.name), blockStateMap } satisfies Scene;
    });
    useStore.getState().setScenes(scenes, Number(d.activeSceneIndex));
  });

  juce.backend.addEventListener('midiMappingsChanged', (detail: unknown) => {
    const d = asRecord(detail);
    const mappings = (Array.isArray(d.mappings) ? d.mappings : []).map((m: unknown) => {
      const r = asRecord(m);
      return {
        channel: Number(r.channel),
        cc: Number(r.cc),
        target: String(r.target),
        blockId: r.blockId ? String(r.blockId) : undefined,
      } satisfies MidiMapping;
    });
    useStore.getState().setMidiMappings(mappings, Boolean(d.learning));
  });

  juce.backend.addEventListener('midiMonitorData', (detail: unknown) => {
    const d = asRecord(detail);
    const events = (Array.isArray(d.events) ? d.events : []).map((e: unknown) => {
      const r = asRecord(e);
      return {
        type: String(r.type),
        channel: Number(r.channel),
        data1: Number(r.data1),
        data2: Number(r.data2),
      } satisfies MidiMonitorEvent;
    });
    if (events.length > 0) {
      const store = useStore.getState();
      store.appendMidiMonitorEvents(events);
      store.updateMidiActivity(events);
    }
  });

  juce.backend.addEventListener('midiLearnComplete', () => {
    // mappingsChanged will follow with the updated list
  });

  juce.backend.addEventListener('tunerData', (detail: unknown) => {
    const d = asRecord(detail);
    useStore
      .getState()
      .setTunerData(
        d.note ? String(d.note) : null,
        Number(d.octave),
        Number(d.cents),
        Number(d.frequency),
        Number(d.confidence),
      );
  });

  juce.backend.addEventListener('systemStats', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setSystemStats(Number(d.cpu), Number(d.outputLevelDb), Boolean(d.clipping));
  });

  juce.backend.addEventListener('sessionSaved', () => {
    useStore.getState().setJustSaved(true);
    setTimeout(() => useStore.getState().setJustSaved(false), 1200);
  });

  juce.backend.addEventListener('referencePitchState', (detail: unknown) => {
    const d = asRecord(detail);
    useStore.getState().setReferencePitch(Number(d.hz));
  });

  juce.backend.addEventListener('telemetryState', (detail: unknown) => {
    const d = asRecord(detail);
    const enabled = Boolean(d.enabled);
    const wasEnabled = useStore.getState().telemetryEnabled;
    useStore.getState().setTelemetryEnabled(enabled);

    const dsn = import.meta.env.VITE_SENTRY_DSN as string | undefined;
    if (enabled && dsn && !Sentry.getClient()) {
      Sentry.init({
        dsn,
        release: `stellarr@${__APP_VERSION__}`,
        environment: (import.meta.env.VITE_SENTRY_ENV as string) ?? 'development',
      });

      // Verify the pipeline works on first opt-in
      if (!wasEnabled) {
        Sentry.captureMessage('Telemetry enabled', 'info');
      }
    }
  });

  juce.backend.addEventListener('blockMetrics', (detail: unknown) => {
    const d = asRecord(detail);
    const rawBlocks = Array.isArray(d.blocks) ? d.blocks : [];
    const samples = rawBlocks.map((b: unknown) => {
      const r = asRecord(b);
      return {
        id: String(r.id),
        lufs: Number(r.lufs),
        targetLufs: r.targetLufs !== undefined ? Number(r.targetLufs) : null,
      };
    });
    useStore.getState().setBlockLufs(samples);

    // Push history sample for currently selected block
    const selectedId = useStore.getState().selectedBlockId;
    const selectedSample = samples.find((s) => s.id === selectedId);
    if (selectedSample) useStore.getState().pushLoudnessSample(selectedSample.lufs);
  });

  juce.backend.addEventListener('lufsWindowState', (detail: unknown) => {
    const d = asRecord(detail);
    const window = String(d.window);
    if (window === 'momentary' || window === 'shortTerm') {
      useStore.getState().setLufsWindow(window);
    }
  });

  bridgeReady = true;
  callNativeFunction('sendToNative', 'bridgeReady', '');
  console.log('[Bridge] TX bridgeReady');
}
