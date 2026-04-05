# Security Policy

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.9.x   | Yes       |

Only the latest release receives security updates.

## Reporting a Vulnerability

If you discover a security vulnerability in Stellarr, please report it responsibly.

**Do not open a public issue.** Instead, email **dev@stellarr.org** with:

- A description of the vulnerability
- Steps to reproduce it
- The version of Stellarr affected
- Any potential impact you've identified

You should receive an acknowledgement within 48 hours. We will work with you to understand the issue and coordinate a fix before any public disclosure.

## Scope

Stellarr is a standalone desktop audio application. The primary security concerns are:

- **Plugin loading** -- Stellarr loads third-party VST3 and Audio Unit plugins, which execute arbitrary code. Only load plugins from trusted sources.
- **Preset files** -- `.stellarr` files contain JSON data including plugin state. Do not open preset files from untrusted sources.
- **WebView bridge** -- The UI runs in a WebView that communicates with the C++ engine. The bridge does not expose network-facing endpoints.

## Out of Scope

- Vulnerabilities in third-party plugins loaded by the user
- Issues requiring physical access to the machine
- Denial of service via audio processing (e.g., feedback loops)
