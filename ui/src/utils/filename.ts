// Filesystem-unsafe character set: the Windows reserved set plus path
// separators and ASCII control codes. Stripping these from any UI-input
// name keeps preset filenames safe on every OS Stellarr might ship on,
// regardless of where the name ends up serialised.
// eslint-disable-next-line no-control-regex
const FILENAME_DISALLOWED = /[<>:"/\\|?*\x00-\x1f]/g;

// Windows reserved basenames — these are off-limits as a filename stem
// even with an extension (e.g. `CON.stellarr` cannot be created on
// Windows). Matched case-insensitively against the stem before the first
// dot. `COM0` and `LPT0` are not reserved on modern Windows but historic
// docs list them; harmless to include.
const RESERVED_BASENAMES = new Set([
  'CON', 'PRN', 'AUX', 'NUL',
  'COM0', 'COM1', 'COM2', 'COM3', 'COM4', 'COM5', 'COM6', 'COM7', 'COM8', 'COM9',
  'LPT0', 'LPT1', 'LPT2', 'LPT3', 'LPT4', 'LPT5', 'LPT6', 'LPT7', 'LPT8', 'LPT9',
]);

/**
 * Strip characters that are unsafe in a cross-platform filesystem path.
 * Applied LIVE to the rename Input so the user only sees valid characters
 * appear; the stripped result is what state stores. Cheap; safe to call
 * on every keystroke.
 */
export function sanitiseFilesystemName(input: string): string {
  return input.replace(FILENAME_DISALLOWED, '');
}

/**
 * Final-pass normalisation applied at submit time. If the basename
 * (everything before the first dot, case-folded) matches a Windows
 * reserved device name like `CON` or `COM1`, append `_` so the rename
 * cannot silently fail on Windows. Run after `.trim()` and after
 * `sanitiseFilesystemName`; idempotent on names that are already safe.
 */
export function ensureSafeBasename(input: string): string {
  const dot = input.indexOf('.');
  const stem = dot === -1 ? input : input.slice(0, dot);
  if (RESERVED_BASENAMES.has(stem.toUpperCase())) {
    return stem + '_' + (dot === -1 ? '' : input.slice(dot));
  }
  return input;
}
