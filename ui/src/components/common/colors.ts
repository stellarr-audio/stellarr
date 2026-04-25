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

  // Grid anchor (Input/Output blocks) — theme-aware via tokens.css.
  gridAnchor: 'var(--grid-anchor-colour)',
} as const;

export type ColorToken = keyof typeof colors;

/**
 * Plugin format identification colours. Used to tint format badges in the
 * plugin picker. Theme-agnostic: a VST3 reads as VST3 in either theme.
 */
export const pluginFormatColors = {
  VST3: 'var(--plugin-format-vst3)',
  AudioUnit: 'var(--plugin-format-au)',
  VST: 'var(--plugin-format-vst)',
  unknown: 'var(--plugin-format-unknown)',
} as const;

/**
 * Block identity palette — user-selectable colours stored on individual grid
 * blocks. These are user data, not design tokens: they persist as raw hex
 * inside `.stellarr` presets, so changing the values here would silently
 * invalidate existing user-coloured blocks.
 *
 * 8 base hues × 2 shades = 16 swatches.
 */
export const blockPalette = [
  '#0077cc',
  '#38bdf8', // blue
  '#6c3fc7',
  '#a78bfa', // violet
  '#cc1259',
  '#f472b6', // pink
  '#cc5500',
  '#ff8800', // orange
  '#b8960a',
  '#ffd43b', // gold
  '#00995c',
  '#00ff9d', // green
  '#008c8c',
  '#00d2d3', // teal
  '#5a6578',
  '#94a3b8', // slate
] as const;

export const blockPaletteByName = {
  blue: blockPalette[0],
  blueLight: blockPalette[1],
  violet: blockPalette[2],
  violetLight: blockPalette[3],
  pink: blockPalette[4],
  pinkLight: blockPalette[5],
  orange: blockPalette[6],
  orangeLight: blockPalette[7],
  gold: blockPalette[8],
  goldLight: blockPalette[9],
  green: blockPalette[10],
  greenLight: blockPalette[11],
  teal: blockPalette[12],
  tealLight: blockPalette[13],
  slate: blockPalette[14],
  slateLight: blockPalette[15],
} as const;
