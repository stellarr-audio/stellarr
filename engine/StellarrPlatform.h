#pragma once

#include <juce_core/juce_core.h>

void stellarrSetDarkAppearance();
void stellarrMakeWebViewInspectable(void* componentPeer);

// Resolve a directory inside the running app's Contents/Resources. The
// shipped DMG bundles UI assets and samples there so the app is
// self-contained; there are no absolute dev-machine paths baked in.
juce::File stellarrGetBundleResource(const juce::String& subpath);
