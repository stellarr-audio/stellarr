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

    void paint(juce::Graphics& g) override
    {
        g.fillAll(kBackgroundColour);

        auto centreX = static_cast<float>(getWidth() / 2);
        auto centreY = static_cast<float>(getHeight() / 2);

        if (logo != nullptr)
        {
            float logoSize = 64.0f;
            logo->setTransformToFit(
                juce::Rectangle<float>(centreX - logoSize / 2.0f,
                                        centreY - 48.0f,
                                        logoSize, logoSize),
                juce::RectanglePlacement::centred);
            logo->draw(g, 1.0f);
        }

        // Matches CSS --color-primary
        g.setColour(juce::Colour(0xffff2d7b));
        g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
        g.drawText("STELLARR",
                   juce::Rectangle<float>(centreX - 100.0f, centreY + 28.0f, 200.0f, 24.0f),
                   juce::Justification::centred);

        // Matches CSS --color-muted
        g.setColour(juce::Colour(90, 84, 120));
        g.setFont(juce::Font(juce::FontOptions(14.0f)));
        g.drawText("Initialising...",
                   juce::Rectangle<float>(centreX - 100.0f, centreY + 58.0f, 200.0f, 20.0f),
                   juce::Justification::centred);
    }

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
