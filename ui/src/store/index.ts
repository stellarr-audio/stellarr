import { create } from 'zustand';

interface StellarrState {
  connected: boolean;
  setConnected: (value: boolean) => void;
}

export const useStore = create<StellarrState>((set) => ({
  connected: false,
  setConnected: (value) => set({ connected: value }),
}));
