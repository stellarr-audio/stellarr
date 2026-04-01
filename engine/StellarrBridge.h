#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <map>

class StellarrProcessor;

class StellarrBridge
{
public:
    StellarrBridge();

    void setProcessor(StellarrProcessor* proc);
    juce::WebBrowserComponent::Options configureOptions(juce::WebBrowserComponent::Options options);
    void setWebView(juce::WebBrowserComponent* browser);

private:
    void handleEvent(const juce::String& eventName, const juce::var& payload);
    void sendWelcome();
    void sendGraphState();

    // Graph event handlers
    void handleAddBlock(const juce::var& json);
    void handleRemoveBlock(const juce::var& json);
    void handleMoveBlock(const juce::var& json);
    void handleAddConnection(const juce::var& json);
    void handleRemoveConnection(const juce::var& json);

    // Plugin management event handlers
    void handleScanPlugins();
    void handleGetScanDirectories();
    void handlePickScanDirectory();
    void handleRemoveScanDirectory(const juce::var& json);

    void sendPluginList();
    void sendScanDirectories();

    void emitToJs(const juce::String& eventName, juce::DynamicObject* detail);

    juce::WebBrowserComponent* webView = nullptr;
    StellarrProcessor* processor = nullptr;

    std::map<juce::String, juce::AudioProcessorGraph::NodeID> blockNodeMap;
    std::map<juce::String, std::pair<int, int>> blockPositions;
};
