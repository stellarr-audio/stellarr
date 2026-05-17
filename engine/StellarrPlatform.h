#pragma once

#include <juce_core/juce_core.h>

void stellarrSetDarkAppearance();

// One-time WebView visual setup — runs after the JUCE WebBrowserComponent's
// peer is attached. Applies the dark background and prevents the white flash
// while the React bundle loads. Idempotent if called more than once.
void stellarrInitWebView(void* componentPeer);

// Toggle the WebView's native Inspect Element / Web Inspector availability.
// Called at init with the persisted developerModeEnabled value, and again
// whenever the user flips the Developer mode switch in System settings.
void stellarrSetWebViewInspectable(void* componentPeer, bool enabled);

// Resolve a directory inside the running app's Contents/Resources. The
// shipped DMG bundles UI assets and samples there so the app is
// self-contained; there are no absolute dev-machine paths baked in.
juce::File stellarrGetBundleResource(const juce::String& subpath);
