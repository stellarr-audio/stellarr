#!/usr/bin/env -S node --experimental-strip-types
/**
 * Prepend a new <item> entry to a Sparkle appcast XML file.
 *
 * Invoked from the release workflow after a new DMG is built, signed
 * (codesign + Sparkle EdDSA), and uploaded. Keeps the newest release
 * first in the channel so clients pick it up on their next check.
 *
 * Usage:
 *   node --experimental-strip-types scripts/update-appcast.ts \
 *     --appcast    <path> \
 *     --version    <x.y.z> \
 *     --url        <dmg-download-url> \
 *     --length     <bytes> \
 *     --signature  <ed25519-signature> \
 *     --pubdate    <rfc-2822-date> \
 *     --notes-url  <release-page-url>
 *
 * Zero npm dependencies on purpose — runs on a clean Node 24 runner.
 */
import { readFileSync, writeFileSync } from 'node:fs';
import { parseArgs } from 'node:util';

interface ItemFields {
  version: string;
  url: string;
  length: string;
  signature: string;
  pubDate: string;
  notesUrl: string;
}

const REQUIRED_FLAGS = [
  'appcast',
  'version',
  'url',
  'length',
  'signature',
  'pubdate',
  'notes-url',
] as const;

function xmlEscape(value: string): string {
  return value
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function renderItem(fields: ItemFields): string {
  const v = xmlEscape(fields.version);
  return [
    '    <item>',
    `      <title>Stellarr ${v}</title>`,
    `      <link>${xmlEscape(fields.notesUrl)}</link>`,
    `      <pubDate>${xmlEscape(fields.pubDate)}</pubDate>`,
    `      <sparkle:version>${v}</sparkle:version>`,
    `      <sparkle:shortVersionString>${v}</sparkle:shortVersionString>`,
    `      <enclosure url="${xmlEscape(fields.url)}"`
      + ` length="${xmlEscape(fields.length)}"`
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
function insertItem(xml: string, itemXml: string): string {
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

function main(): void {
  const { values } = parseArgs({
    options: {
      appcast:     { type: 'string' },
      version:     { type: 'string' },
      url:         { type: 'string' },
      length:      { type: 'string' },
      signature:   { type: 'string' },
      pubdate:     { type: 'string' },
      'notes-url': { type: 'string' },
    },
  });

  for (const flag of REQUIRED_FLAGS) {
    if (!values[flag]) {
      console.error(`Missing required flag: --${flag}`);
      process.exit(1);
    }
  }

  const path = values.appcast as string;
  const xml = readFileSync(path, 'utf8');
  const itemXml = renderItem({
    version:   values.version   as string,
    url:       values.url       as string,
    length:    values.length    as string,
    signature: values.signature as string,
    pubDate:   values.pubdate   as string,
    notesUrl:  values['notes-url'] as string,
  });

  writeFileSync(path, insertItem(xml, itemXml));
}

main();
