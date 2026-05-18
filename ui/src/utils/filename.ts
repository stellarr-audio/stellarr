// Filesystem-unsafe character set: the Windows reserved set plus path
// separators and ASCII control codes. Stripping these from any UI-input
// name keeps preset filenames safe on every OS Stellarr might ship on,
// regardless of where the name ends up serialised.
// eslint-disable-next-line no-control-regex
const FILENAME_DISALLOWED = /[<>:"/\\|?*\x00-\x1f]/g;

/**
 * Strip characters that are unsafe in a cross-platform filesystem path.
 * Applied to preset/scene rename inputs so the user can only type valid
 * characters; the stripped result is what state stores.
 */
export function sanitiseFilesystemName(input: string): string {
  return input.replace(FILENAME_DISALLOWED, '');
}
