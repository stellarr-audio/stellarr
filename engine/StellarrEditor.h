#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "StellarrBridge.h"

class StellarrProcessor;

// Matches CSS --color-bg (#0d0b1a)
static const juce::Colour kBackgroundColour { 13, 11, 26 };

// Full-screen overlay shown while the WebView and React UI load.
// Removed by StellarrEditor::hideSplash() when JS emits "uiReady".
class SplashOverlay : public juce::Component
{
public:
    void setLogo(std::unique_ptr<juce::Drawable> d) { logo = std::move(d); }
    void paint(juce::Graphics& g) override;

private:
    std::unique_ptr<juce::Drawable> logo;
};

class StellarrEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit StellarrEditor(StellarrProcessor&);
    ~StellarrEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Apply the developer-tools state to the WebView: toggles native Inspect
    // Element availability and the right-click `oncontextmenu` interception.
    // Called at startup with the persisted `developerModeEnabled` value and
    // again whenever the user flips the Developer mode switch in System
    // settings (PresetHandler routes the toggle through this method).
    void setDevToolsEnabled(bool enabled);

    void hideSplash();
    StellarrBridge& getBridge() { return bridge; }

private:
    static juce::String getMimeType(const juce::File& file);
    void timerCallback() override;
    void applyContextMenuInterception();
    void applyWebViewInspectable();

    StellarrBridge bridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<SplashOverlay> splashOverlay;
    int timerTick = 0;
    bool devToolsEnabled = false;
};
