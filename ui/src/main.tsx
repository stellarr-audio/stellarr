import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import './design/tokens.css';
import './assets/fonts/fonts.css';
import './styles/variables.css';
import { initBridge } from './bridge';
import App from './App';

initBridge();

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>,
);
