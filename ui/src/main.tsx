import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './design/tokens.css';
import './assets/fonts/fonts.css';
import './styles/variables.css';
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
