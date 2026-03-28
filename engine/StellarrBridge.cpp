#include "StellarrBridge.h"

StellarrBridge::StellarrBridge() = default;

juce::WebBrowserComponent::Options StellarrBridge::configureOptions(juce::WebBrowserComponent::Options options)
{
    return options.withNativeFunction("sendToNative",
        [this](const juce::Array<juce::var>& args,
               juce::WebBrowserComponent::NativeFunctionCompletion completion)
        {
            if (args.size() >= 2)
                handleEvent(args[0].toString(), args[1]);
            else if (args.size() == 1)
                handleEvent(args[0].toString(), {});

            completion(juce::var("ok"));
        });
}

void StellarrBridge::setWebView(juce::WebBrowserComponent* browser)
{
    webView = browser;
}

void StellarrBridge::handleEvent(const juce::String& eventName, const juce::var& payload)
{
    DBG("Bridge received from JS: " + eventName + " — " + payload.toString());

    if (eventName == "bridgeReady")
        sendWelcome();

    if (eventName == "uiAction" && webView != nullptr)
    {
        auto* response = new juce::DynamicObject();
        response->setProperty("message", "Pong: " + payload.toString());
        webView->emitEventIfBrowserIsVisible("pong", juce::var(response));
    }
}

void StellarrBridge::sendWelcome()
{
    if (webView == nullptr)
        return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("message", "Stellarr C++ engine is running");

    webView->emitEventIfBrowserIsVisible("welcome", juce::var(detail));
    DBG("Bridge sent welcome event to JS");
}
