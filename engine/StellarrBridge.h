#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

class StellarrBridge
{
public:
    StellarrBridge();

    juce::WebBrowserComponent::Options configureOptions(juce::WebBrowserComponent::Options options);
    void setWebView(juce::WebBrowserComponent* browser);

private:
    void handleEvent(const juce::String& eventName, const juce::var& payload);
    void sendWelcome();

    juce::WebBrowserComponent* webView = nullptr;
};
