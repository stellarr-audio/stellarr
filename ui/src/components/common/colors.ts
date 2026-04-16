/**
 * Re-exports design tokens as CSS variable references for use in JS/TS
 * (e.g. inline styles, canvas / SVG fill attributes).
 *
 * The *values* live in `ui/src/design/tokens.css`. This module only provides
 * the `var(--…)` strings. Do NOT hardcode hex colours here — use tokens.
 */

export const colors = {
  // Surfaces
  bg: 'var(--bg)',
  surface: 'var(--surface)',
  cell: 'var(--surface)',
  cellHover: 'var(--surface)',
  border: 'var(--border)',
  blockBg: 'var(--surface)',
  blockBorder: 'var(--border)',
  dropdownBg: 'var(--surface)',

  // Text
  text: 'var(--text)',
  muted: 'var(--text-muted)',

  // Accents
  primary: 'var(--accent)',
  secondary: 'var(--secondary)',
  portColor: 'var(--secondary)',

  // Semantic
  green: 'var(--success)',
  danger: 'var(--danger)',
  dangerBright: 'var(--danger)',
  warning: 'var(--secondary)',
  connectionLine: 'var(--text-muted)',
} as const;

export type ColorToken = keyof typeof colors;
