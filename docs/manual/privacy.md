---
title: Privacy & Telemetry
description: What Stellarr does and doesn't collect.
sidebar:
  order: 9
---

Stellarr respects your privacy. Crash reporting is **off by default** and only activates when you explicitly opt in.

## Opt-in Crash Reporting

When enabled, Stellarr sends anonymous crash reports to help us find and fix bugs proactively. This uses [Sentry](https://sentry.io), an open-source error tracking platform.

### How to enable or disable

1. Go to the **System** tab.
2. Under **Privacy**, toggle **Send crash reports**.
3. The setting takes effect immediately for native crash reporting, and on the next app launch for UI error reporting.

### What is collected

When a crash or error occurs, the following is sent:

- **Stack trace** -- The sequence of function calls that led to the crash. This tells us where in the code the problem happened.
- **Error message** -- A description of what went wrong (e.g., "null pointer access").
- **App version** -- Which version of Stellarr you are running.
- **Operating system** -- macOS version (e.g., "macOS 15.3").
- **Device architecture** -- CPU type (e.g., "arm64").
- **Anonymous event ID** -- A random identifier for the crash event itself, not linked to your identity.

### What is never collected

- Your name, email, IP address, or any account information
- Plugin names or identifiers (what plugins you use is your business)
- Preset file names or content
- Audio or MIDI data
- File system paths
- Usage patterns, analytics, or behavioural data

### Where data is stored

Crash reports are sent to a Sentry project hosted by Sentry.io. Data is retained according to Sentry's [data retention policy](https://sentry.io/security/) and is used solely for debugging Stellarr.

### When data is sent

Data is only sent at the moment a crash or unhandled error occurs. Stellarr does not send any data during normal operation, even when telemetry is enabled.

## No Telemetry Without Consent

If you do not enable crash reporting:

- No crash data is sent to Sentry or any other external service
- No data leaves your machine beyond the unavoidable update-check traffic described below

## Software Update Checks

Stellarr checks [stellarr.org/appcast.xml](https://stellarr.org/appcast.xml) on launch and once a day while running to see whether a newer version is available. See [Software Updates](/docs/system/#software-updates) for the user-facing controls.

### What the update check sends

Each check is a plain HTTP GET to the appcast URL. The server (stellarr.org, hosted on GitHub Pages) can see the minimum any web server sees:

- **Your IP address** — standard for any HTTP request.
- **A Sparkle user-agent string** — identifies the update framework version.
- **The current Stellarr app version** — sent in the user-agent, so the server could serve version-targeted responses. Stellarr today serves the same appcast to everyone.

The same is true of the DMG download itself, which is fetched from the GitHub Releases CDN when you click **Install update**.

### What the update check does NOT send

Stellarr deliberately disables Sparkle's optional "system profile" feature. We never send:

- CPU type, model identifier, or hardware information
- Operating system version (beyond what any browser UA would reveal)
- Preferred language
- Plugin counts, preset usage, or any in-app state
- Any information about what you do with the app

This is enforced both in the app's Info.plist (no `SUSendProfileInfo` key) and at runtime — the update driver hard-codes `sendSystemProfile: NO` when Sparkle asks.

### Turning checks off

Checks happen automatically but nothing is downloaded or installed without you pressing a button. If you want to disable the check itself, quit Stellarr and run:

```
defaults write com.stellarr.stellarr SUEnableAutomaticChecks -bool NO
```

(Use `com.stellarr.stellarr.dev` for a dev build.) You can still use **Check for updates** in Settings to check manually.

## Website Analytics

The Stellarr website (stellarr.org and its docs) uses [GoatCounter](https://www.goatcounter.com) for basic visitor counting.

What GoatCounter does:

- Counts page views and referrers
- **No cookies**, **no fingerprinting**, **no personal data**
- GDPR-compliant without requiring a consent banner
- Retains a hashed, salted proxy of the IP for 8 hours (for deduplication), never the raw IP

What it does not do: track you across sessions, build a profile, share with third parties, or sell data.

The app itself never talks to GoatCounter — this is purely website analytics.

## Open Source

Stellarr is open source. You can inspect the telemetry implementation yourself in the [source code](https://github.com/stellarr-audio/stellarr) to verify these claims.
