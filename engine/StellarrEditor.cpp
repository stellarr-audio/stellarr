#include "StellarrEditor.h"
#include "StellarrProcessor.h"
#include "StellarrPlatform.h"

StellarrEditor::StellarrEditor(StellarrProcessor& p)
    : AudioProcessorEditor(p)
{
    stellarrSetDarkAppearance();
    auto uiDir = juce::File(STELLARR_UI_DIR);

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
    bridge.setOnStartupComplete([this]() { hideSplash(); });

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Show logo on top of WebView until page loads
    auto svgFile = uiDir.getChildFile("logo.svg");
    if (svgFile.existsAsFile())
    {
        auto xml = juce::XmlDocument::parse(svgFile);
        if (xml != nullptr)
        {
            if (auto drawable = juce::Drawable::createFromSVG(*xml))
            {
                splashLogo = std::make_unique<juce::DrawableComposite>();
                splashLogo->addAndMakeVisible(drawable.release());
                addAndMakeVisible(*splashLogo);
                splashLogo->setAlwaysOnTop(true);
            }
        }
    }

    setResizable(true, true);
    setSize(1440, 700);
    setResizeLimits(1440, 700, 2560, 1600);

    juce::MessageManager::callAsync(
        [safeWebView = juce::Component::SafePointer(webView.get())]()
        {
            if (safeWebView != nullptr)
                if (auto* peer = safeWebView->getTopLevelComponent()->getPeer())
                    stellarrMakeWebViewInspectable(peer->getNativeHandle());
        });

    startTimerHz(20);
}

StellarrEditor::~StellarrEditor()
{
    stopTimer();
}

void StellarrEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(82, 77, 115));
}

void StellarrEditor::resized()
{
    webView->setBounds(getLocalBounds());

    if (splashLogo != nullptr)
    {
        int logoSize = 48;
        auto area = getLocalBounds();
        splashLogo->setBounds(area.getCentreX() - logoSize / 2,
                              area.getCentreY() - logoSize / 2,
                              logoSize, logoSize);
    }
}

void StellarrEditor::timerCallback()
{
    ++timerTick;

    // System stats at ~2Hz (every 10th tick)
    if (timerTick % 10 == 0)
    {
        auto& proc = dynamic_cast<StellarrProcessor&>(processor);
        bridge.sendSystemStats(proc.getCpuUsagePercent(),
                               stellarrGetProcessMemoryMB(),
                               stellarrGetTotalMemoryMB());
    }

    // Tuner data at ~20Hz
    if (bridge.isTunerActive())
        bridge.sendTunerData();
}

void StellarrEditor::toggleDevTools()
{
    devToolsEnabled = !devToolsEnabled;

    if (devToolsEnabled)
        webView->evaluateJavascript("document.body.oncontextmenu = null;");
    else
        webView->evaluateJavascript("document.body.oncontextmenu = function() { return false; };");
}

bool StellarrEditor::isDevToolsEnabled() const { return devToolsEnabled; }

void StellarrEditor::hideSplash()
{
    if (splashLogo != nullptr)
    {
        removeChildComponent(splashLogo.get());
        splashLogo = nullptr;
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
