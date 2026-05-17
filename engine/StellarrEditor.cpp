#include "StellarrEditor.h"
#include "StellarrProcessor.h"
#include "StellarrPlatform.h"

StellarrEditor::StellarrEditor(StellarrProcessor& p)
    : AudioProcessorEditor(p)
{
    stellarrSetDarkAppearance();
    auto uiDir = stellarrGetBundleResource("ui");

    auto options = bridge.configureOptions(
        juce::WebBrowserComponent::Options{}
            .withResourceProvider(
                [uiDir](const juce::String& path)
                    -> std::optional<juce::WebBrowserComponent::Resource>
                {
                    auto resolved = path;

                    if (resolved.isEmpty() || resolved == "/")
                        resolved = "/index.html";

                    auto file = uiDir.getChildFile(resolved.substring(1));

                    if (!file.existsAsFile())
                        return std::nullopt;

                    juce::MemoryBlock data;
                    file.loadFileAsData(data);

                    std::vector<std::byte> bytes(data.getSize());
                    std::memcpy(bytes.data(), data.getData(), data.getSize());

                    return juce::WebBrowserComponent::Resource{
                        std::move(bytes), getMimeType(file)};
                }));

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);
    bridge.setProcessor(&p);
    bridge.setAppProperties(p.getAppProperties());
    bridge.setWebView(webView.get());

    // Seed dev-tools state from the persisted setting so the WebView is
    // gated from the first frame rather than after the UI finishes loading.
    // Default OFF matches the production-usage-first design rule
    // (CLAUDE.md: dev / debug / experimental surfaces hidden by default).
    devToolsEnabled = p.getAppProperties()->getUserSettings()
                       ->getBoolValue("developerModeEnabled", false);

    bridge.setOnDevToolsToggle([this](bool enabled) { setDevToolsEnabled(enabled); });

    bridge.setOnUiReady([this]()
    {
        hideSplash();
        // document.body now exists — apply the right-click interception
        // matching the persisted dev-tools state. Subsequent toggles also
        // re-apply via setDevToolsEnabled().
        applyContextMenuInterception();
    });

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Splash overlay on top of WebView
    splashOverlay = std::make_unique<SplashOverlay>();
    auto svgFile = uiDir.getChildFile("logo.svg");
    if (svgFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(svgFile);
        if (xml != nullptr)
            splashOverlay->setLogo(juce::Drawable::createFromSVG(*xml));
    }
    addAndMakeVisible(*splashOverlay);
    splashOverlay->setAlwaysOnTop(true);

    setResizable(true, true);
    setSize(1440, 830);
    setResizeLimits(1440, 830, 2560, 1600);

    juce::MessageManager::callAsync(
        [safeWebView = juce::Component::SafePointer(webView.get()),
         initialDevTools = devToolsEnabled]()
        {
            if (safeWebView != nullptr)
                if (auto* peer = safeWebView->getTopLevelComponent()->getPeer())
                {
                    auto* nativeHandle = peer->getNativeHandle();
                    stellarrInitWebView(nativeHandle);
                    stellarrSetWebViewInspectable(nativeHandle, initialDevTools);
                }
        });

    startTimerHz(20);
}

StellarrEditor::~StellarrEditor()
{
    stopTimer();
}

void SplashOverlay::paint(juce::Graphics& g)
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

void StellarrEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBackgroundColour);
}

void StellarrEditor::resized()
{
    webView->setBounds(getLocalBounds());

    if (splashOverlay != nullptr)
        splashOverlay->setBounds(getLocalBounds());
}

void StellarrEditor::timerCallback()
{
    ++timerTick;

    // System stats at ~4Hz (every 5th tick) — fast enough for responsive level meter
    if (timerTick % 5 == 0)
    {
        auto& proc = dynamic_cast<StellarrProcessor&>(processor);
        bridge.sendSystemStats(proc.getCpuUsagePercent(),
                               proc.getOutputPeakLevel());
        bridge.sendBlockMetrics();
    }

    // Tuner data at ~20Hz
    if (bridge.isTunerActive())
        bridge.sendTunerData();

    // MIDI monitor at ~20Hz
    bridge.sendMidiMonitorData();

    // Drain mapped-MIDI events from the audio thread and dispatch the
    // configured callbacks on the message thread (~20Hz).
    bridge.drainMidiEvents();
}

void StellarrEditor::setDevToolsEnabled(bool enabled)
{
    devToolsEnabled = enabled;
    applyWebViewInspectable();
    applyContextMenuInterception();
}

void StellarrEditor::applyContextMenuInterception()
{
    if (webView == nullptr)
        return;

    // Suppress the native right-click menu (Inspect Element / Reload / etc.)
    // when developer mode is off, so first-launch users only see what they
    // expect from a normal app.
    if (devToolsEnabled)
        webView->evaluateJavascript("document.body.oncontextmenu = null;");
    else
        webView->evaluateJavascript("document.body.oncontextmenu = function() { return false; };");
}

void StellarrEditor::applyWebViewInspectable()
{
    if (webView == nullptr)
        return;

    if (auto* peer = getTopLevelComponent()->getPeer())
        stellarrSetWebViewInspectable(peer->getNativeHandle(), devToolsEnabled);
}

void StellarrEditor::hideSplash()
{
    if (splashOverlay != nullptr)
    {
        removeChildComponent(splashOverlay.get());
        splashOverlay = nullptr;
    }
}

juce::String StellarrEditor::getMimeType(const juce::File& file)
{
    auto ext = file.getFileExtension().toLowerCase();

    if (ext == ".html")                return "text/html";
    if (ext == ".js")                  return "application/javascript";
    if (ext == ".css")                 return "text/css";
    if (ext == ".json")                return "application/json";
    if (ext == ".svg")                 return "image/svg+xml";
    if (ext == ".png")                 return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".woff2")               return "font/woff2";
    if (ext == ".woff")                return "font/woff";

    return "application/octet-stream";
}
