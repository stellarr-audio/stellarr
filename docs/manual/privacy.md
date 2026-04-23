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
- No network requests are made
- No data leaves your machine

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
