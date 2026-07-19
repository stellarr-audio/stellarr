// react-scan — runtime "why did this re-render" instrumentation. Loaded
// dynamically inside the dev-only gate so the production bundle
// tree-shakes the package out entirely. Workflow + verification grep
// documented in CLAUDE.md → "How to profile React renders".
if (import.meta.env.DEV) {
  void import('react-scan').then(({ scan }) => scan({ enabled: true }));
}

import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './design/tokens.css';
import './assets/fonts/fonts.css';
import './styles/variables.css';
import './styles/base.css';
import { initBridge } from './bridge';
import { useStore } from './store';
import App from './App';

initBridge();

// Expose the Zustand store on window for devtools-driven debugging. The app
// is a local WebView with no untrusted surface, so there is no security cost.
(window as unknown as { useStore: typeof useStore }).useStore = useStore;

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>,
);
