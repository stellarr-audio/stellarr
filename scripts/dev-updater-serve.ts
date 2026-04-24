#!/usr/bin/env -S node --experimental-strip-types
/**
 * Local Sparkle update harness.
 *
 * Given a DMG and a version string, signs the DMG with the dev EdDSA
 * private key, generates a one-item appcast, and serves both over HTTP
 * so a running Stellarr Dev build (pointed at the local SUFeedURL) can
 * exercise the full check → download → install → relaunch flow.
 *
 * Usage:
 *   node --experimental-strip-types scripts/dev-updater-serve.ts \
 *     --dmg <path-to-dmg> --version 0.99.1 [--port 8765]
 *
 * Prerequisites:
 *   - make regen-sparkle-keys-dev has been run at least once so
 *     ~/.stellarr/dev-updater/priv exists.
 *   - cmake -B build-dev has fetched Sparkle so sign_update is on disk.
 */
import { createServer } from 'node:http';
import { createReadStream, existsSync, mkdirSync, mkdtempSync, readFileSync, statSync, writeFileSync } from 'node:fs';
import { basename, join, resolve, sep } from 'node:path';
import { execFileSync } from 'node:child_process';
import { tmpdir, homedir } from 'node:os';
import { parseArgs } from 'node:util';
import { buildAppcast } from './lib/appcast.ts';

interface Paths {
  devPrivKey:  string;
  signUpdate:  string;
}

function resolvePaths(): Paths {
  const devPrivKey = join(homedir(), '.stellarr', 'dev-updater', 'priv');
  if (!existsSync(devPrivKey)) {
    throw new Error(
      `Dev private key not found at ${devPrivKey}. Run \`make regen-sparkle-keys-dev\` first.`,
    );
  }

  const candidates = [
    'build-dev/_deps/sparkle-src/bin/sign_update',
    'build-prod/_deps/sparkle-src/bin/sign_update',
    'build/_deps/sparkle-src/bin/sign_update',
  ];
  const signUpdate = candidates.find((p) => existsSync(p));
  if (!signUpdate) {
    throw new Error(
      'sign_update not found. Run `cmake -B build-dev` (or build-prod) first so Sparkle is fetched.',
    );
  }

  return { devPrivKey, signUpdate: resolve(signUpdate) };
}

function signArtefact(signUpdate: string, artefact: string, privKey: string): string {
  // -p prints the bare EdDSA signature (suitable for automation). Without
  // it sign_update emits a full XML attribute fragment instead.
  return execFileSync(signUpdate, ['-p', artefact, '--ed-key-file', privKey], { encoding: 'utf8' }).trim();
}

/**
 * Sparkle signs single-file archives (DMG, ZIP, tar.*), not directories.
 * If the caller passes a .app bundle, wrap it in a ZIP on the fly via
 * macOS `ditto`, which preserves metadata and resource forks.
 */
function ensureArchive(input: string, stagingDir: string): string {
  if (statSync(input).isFile()) return input;

  if (!input.endsWith('.app')) {
    throw new Error(
      `Input is a directory but not an .app bundle: ${input}`,
    );
  }

  const archiveName = `${basename(input, '.app')}.zip`;
  const archivePath = join(stagingDir, archiveName);
  execFileSync('ditto', ['-c', '-k', '--sequesterRsrc', '--keepParent', input, archivePath]);
  return archivePath;
}

function rfc2822Now(): string {
  // Sparkle accepts RFC 2822 / RFC 822 dates. Build one deterministically
  // in the C locale so weekday/month names are correct.
  const d = new Date();
  const days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
  const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
  const pad = (n: number) => String(n).padStart(2, '0');
  return `${days[d.getUTCDay()]}, ${pad(d.getUTCDate())} ${months[d.getUTCMonth()]} ${d.getUTCFullYear()} `
       + `${pad(d.getUTCHours())}:${pad(d.getUTCMinutes())}:${pad(d.getUTCSeconds())} +0000`;
}

function prepareServeDir(
  artefact: string,
  version: string,
  signature: string,
  port: number,
): string {
  const serveDir = join(tmpdir(), `stellarr-dev-updater-${Date.now()}`);
  mkdirSync(serveDir, { recursive: true });

  const artefactName = basename(artefact);
  const artefactDest = join(serveDir, artefactName);
  writeFileSync(artefactDest, readFileSync(artefact));

  const size = statSync(artefactDest).size;
  const feedUrl = `http://localhost:${port}/appcast.xml`;
  const notesUrl = `http://localhost:${port}/release-notes.html`;
  const artefactUrl = `http://localhost:${port}/${encodeURIComponent(artefactName)}`;

  const appcast = buildAppcast('Stellarr (local dev)', feedUrl, {
    version,
    url:       artefactUrl,
    length:    size,
    signature,
    pubDate:   rfc2822Now(),
    notesUrl,
  });
  writeFileSync(join(serveDir, 'appcast.xml'), appcast);

  const notes = `<!doctype html><meta charset="utf-8"><title>Stellarr ${version}</title>`
              + `<h1>Stellarr ${version}</h1>`
              + `<p>Local development release notes placeholder.</p>`;
  writeFileSync(join(serveDir, 'release-notes.html'), notes);

  return serveDir;
}

function startServer(serveDir: string, port: number): void {
  const mime: Record<string, string> = {
    '.xml':  'application/xml; charset=utf-8',
    '.html': 'text/html; charset=utf-8',
    '.dmg':  'application/octet-stream',
  };

  const rootDir = resolve(serveDir);

  const server = createServer((req, res) => {
    const urlPath = decodeURIComponent((req.url ?? '/').split('?')[0]);
    const requested = urlPath === '/' ? 'appcast.xml' : urlPath.replace(/^\/+/, '');

    // Containment check: resolve the requested path to an absolute path,
    // then confirm it still lives inside the serve root. Covers ../,
    // URL-encoded traversal, and absolute-URL cases.
    const candidate = resolve(rootDir, requested);
    if (candidate !== rootDir && !candidate.startsWith(rootDir + sep)) {
      res.writeHead(403);
      res.end('Forbidden');
      return;
    }

    if (!existsSync(candidate) || !statSync(candidate).isFile()) {
      res.writeHead(404);
      res.end('Not found');
      return;
    }

    const ext = candidate.slice(candidate.lastIndexOf('.'));
    res.writeHead(200, { 'Content-Type': mime[ext] ?? 'application/octet-stream' });
    createReadStream(candidate).pipe(res);
  });

  server.listen(port, 'localhost', () => {
    console.log(`\nServing local appcast at http://localhost:${port}/appcast.xml`);
    console.log(`\nPoint a Stellarr Dev build at this feed, then trigger Check for Updates:`);
    console.log(`  defaults write com.stellarr.stellarr.dev SUFeedURL "http://localhost:${port}/appcast.xml"`);
    console.log(`\nTo reset:`);
    console.log(`  defaults delete com.stellarr.stellarr.dev SUFeedURL\n`);
    console.log(`Ctrl-C to stop.`);
  });
}

function main(): void {
  const { values } = parseArgs({
    options: {
      dmg:     { type: 'string' },
      version: { type: 'string' },
      port:    { type: 'string', default: '8765' },
    },
  });

  if (!values.dmg || !values.version) {
    console.error('Usage: dev-updater-serve.ts --dmg <path> --version <x.y.z> [--port <n>]');
    console.error('  --dmg accepts a .dmg, .zip, .tar.xz, or an .app bundle (auto-zipped).');
    process.exit(1);
  }

  const inputPath = resolve(values.dmg);
  if (!existsSync(inputPath)) {
    console.error(`Input not found: ${inputPath}`);
    process.exit(1);
  }

  const port = Number.parseInt(values.port ?? '8765', 10);
  if (!Number.isInteger(port) || port < 1 || port > 65_535) {
    console.error(`Invalid port: ${values.port}`);
    process.exit(1);
  }

  const stagingDir = mkdtempSync(join(tmpdir(), 'stellarr-dev-staging-'));
  const artefact = ensureArchive(inputPath, stagingDir);

  const { devPrivKey, signUpdate } = resolvePaths();
  const signature = signArtefact(signUpdate, artefact, devPrivKey);
  const serveDir = prepareServeDir(artefact, values.version, signature, port);
  startServer(serveDir, port);
}

main();
