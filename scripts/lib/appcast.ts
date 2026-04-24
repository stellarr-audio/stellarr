/**
 * Shared Sparkle appcast helpers. Used by scripts/update-appcast.ts (CI)
 * and scripts/dev-updater-serve.ts (local test harness).
 *
 * Zero npm dependencies on purpose — runs on Node 24's native type
 * stripping without any install step.
 */

export interface ItemFields {
  version:   string;
  url:       string;
  length:    string | number;
  signature: string;
  pubDate:   string;
  notesUrl:  string;
}

export function xmlEscape(value: string): string {
  return value
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

/**
 * Render a single <item> block with leading whitespace matching the rest
 * of the appcast file, and a trailing newline so consecutive inserts don't
 * run together.
 */
export function renderItem(fields: ItemFields): string {
  const v = xmlEscape(fields.version);
  return [
    '    <item>',
    `      <title>Stellarr ${v}</title>`,
    `      <link>${xmlEscape(fields.notesUrl)}</link>`,
    `      <pubDate>${xmlEscape(fields.pubDate)}</pubDate>`,
    `      <sparkle:version>${v}</sparkle:version>`,
    `      <sparkle:shortVersionString>${v}</sparkle:shortVersionString>`,
    `      <enclosure url="${xmlEscape(fields.url)}"`
      + ` length="${xmlEscape(String(fields.length))}"`
      + ` type="application/octet-stream"`
      + ` sparkle:edSignature="${xmlEscape(fields.signature)}"/>`,
    '    </item>',
    '',
  ].join('\n');
}

/**
 * Structural insertion: prefer inserting before the first existing <item>
 * (so the new release sits at the top); otherwise insert before </channel>.
 * No marker comments required — the appcast file stays clean.
 */
export function insertItem(xml: string, itemXml: string): string {
  const anchorIdx = (() => {
    const firstItem = xml.indexOf('<item>');
    if (firstItem !== -1) return firstItem;
    const closeChannel = xml.indexOf('</channel>');
    if (closeChannel === -1) {
      throw new Error('Appcast has no </channel> element');
    }
    return closeChannel;
  })();

  const lineStart = xml.lastIndexOf('\n', anchorIdx) + 1;
  return xml.slice(0, lineStart) + itemXml + xml.slice(lineStart);
}

/**
 * Wrap a single <item> in a minimal standalone appcast document. Used by
 * the local test harness which creates an ephemeral appcast from scratch
 * rather than prepending to an existing file.
 */
export function buildAppcast(channelTitle: string, feedUrl: string, item: ItemFields): string {
  const itemXml = renderItem(item);
  return [
    '<?xml version="1.0" standalone="yes"?>',
    '<rss xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle" version="2.0">',
    '  <channel>',
    `    <title>${xmlEscape(channelTitle)}</title>`,
    `    <link>${xmlEscape(feedUrl)}</link>`,
    '    <description>Local Sparkle appcast for development testing.</description>',
    '    <language>en</language>',
    itemXml.trimEnd(),
    '  </channel>',
    '</rss>',
    '',
  ].join('\n');
}
