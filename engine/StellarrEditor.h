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

    void toggleDevTools();
    bool isDevToolsEnabled() const;
    void hideSplash();

private:
    static juce::String getMimeType(const juce::File& file);
    void timerCallback() override;

    StellarrBridge bridge;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    std::unique_ptr<SplashOverlay> splashOverlay;
    int timerTick = 0;
    bool devToolsEnabled = false;
};
