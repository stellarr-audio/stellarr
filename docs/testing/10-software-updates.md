---
title: Software Updates
sidebar:
  order: 10
---

Test whenever you touch the Sparkle integration, `UpdaterShim`, `UpdateHandler`, the `SoftwareUpdates` UI, the release workflow, or the appcast helpers.

## Prerequisites

- `make regen-sparkle-keys-dev` has been run at least once so `~/.stellarr/dev-updater/priv` exists.
- `cmake -B build-dev` has been run at least once so Sparkle is fetched under `build-dev/_deps/sparkle-src/`.
- A built dev app bundle at `build-dev/Stellarr_artefacts/Debug/Standalone/Stellarr Dev.app`.

## Test Cases

### TC-UPD-001: Harness-driven check → banner → install flow

Exercises the full pipeline end-to-end against the local harness (see [system](/docs/system/#software-updates) for the user-facing explanation).

**Steps:**

1. Point the dev app at the local harness:
   ```bash
   defaults write com.stellarr.stellarr.dev SUFeedURL "http://localhost:8765/appcast.xml"
   ```
2. Start the harness in one terminal, pointing it at the dev app bundle:
   ```bash
   make dev-updater-serve \
     DMG="build-dev/Stellarr_artefacts/Debug/Standalone/Stellarr Dev.app" \
     VERSION=0.99.1
   ```
3. Launch Stellarr Dev.
4. Open Settings → Software Updates.
5. Click **Check for Updates**.
6. Click **Download & Install**.

**Expected:**

- Check button briefly shows "Checking…" (disabled).
- Amber banner appears: "Update available v0.99.1 · Released today".
- Amber dot badge appears on the System tab icon.
- "View release notes →" opens the local HTML placeholder in the default browser.
- **Download & Install** triggers a download; right-hand button label flips to "Downloading… X%" and disables itself. The left "Check for Updates" button also disables while a download is in flight.
- Once the download completes the right button becomes **Restart Now**, and a line underneath reads "Update will install when you quit or restart Stellarr."
- Sparkle validates the EdDSA signature against the baked public key (dev key) and accepts the archive.

**Notes:** The harness currently zips the dev `.app` and serves the SAME version back as "v0.99.1". Sparkle will download and signature-verify correctly, but refuses to install-and-relaunch because the archive's `CFBundleShortVersionString` doesn't match the advertised version. That refusal is correct behaviour — it validates the version-guard without needing a second build.

### TC-UPD-002: Up-to-date state

**Steps:**

1. Stop the harness (Ctrl-C).
2. Reset the feed URL:
   ```bash
   defaults delete com.stellarr.stellarr.dev SUFeedURL
   ```
3. Relaunch Stellarr Dev.
4. Settings → Software Updates → Check for Updates.

**Expected:** Banner stays hidden. Status line reads "You're on v0.13.0, the latest version." with a green marker and persists after the Sparkle session ends. Tab badge absent.

**Notes:** This relies on the staging appcast (`stellarr.org/appcast-staging.xml`) existing and containing no newer version. If the feed is unreachable, status flips to error instead — that is TC-UPD-003.

### TC-UPD-003: Offline / error state

**Steps:**

1. Disable internet (Wi-Fi off, or block `stellarr.org` with a hosts entry).
2. Relaunch Stellarr Dev.
3. Settings → Software Updates → Check for Updates.

**Expected:** A rose-coloured error line appears under the blurb (e.g. "A network connection could not be established."). No banner, no badge.

### TC-UPD-004: Release notes open in external browser

**Steps:**

1. With the harness running and an "Update available" banner visible (TC-UPD-001 step 5).
2. Click **View release notes →**.

**Expected:** Default browser opens the notes URL. The Stellarr Dev window remains focused afterwards (no app switch flicker). No in-app navigation.

### TC-UPD-005: Full install-and-relaunch (real two-version rehearsal)

Requires two genuinely different dev releases — typically cut before a real minor-version release to rehearse the staging feed.

**Steps:**

1. Produce a dev release at version X.Y.Z:
   ```bash
   STELLARR_FLAVOUR=dev make release
   ```
2. Bump `ui/package.json` to X.Y.Z+1 and produce a second dev release.
3. Point the harness at the NEW DMG:
   ```bash
   make dev-updater-serve DMG=<path-to-new.dmg> VERSION=X.Y.Z+1
   ```
4. Install the OLD dev app (X.Y.Z) into `/Applications`.
5. Launch it, Settings → Software Updates → Download & Install.
6. When the button flips to **Restart Now**, click it (or quit the app by any means — the staged install runs on next termination regardless).

**Expected:** Sparkle installs the new bundle, Stellarr Dev quits and relaunches showing version X.Y.Z+1 in the title bar. Tab badge clears. Status line reads "You're on vX.Y.Z+1, the latest version."

**Notes:** Run this before cutting any release that ships Sparkle changes.

### TC-UPD-006: Production build trust model

**Steps:**

1. Build a prod flavour app locally: `make release` (no `STELLARR_FLAVOUR=dev`).
2. Inspect `Info.plist`:
   ```bash
   /usr/libexec/PlistBuddy -c "Print :SUPublicEDKey" \
     build-release/Stellarr_artefacts/Release/Standalone/Stellarr.app/Contents/Info.plist
   ```

**Expected:** Matches `STELLARR_SUPUBLIC_EDKEY_PROD` in `CMakeLists.txt`, which is the Keychain-held prod public key. A dev-signed update would fail verification.
