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

    void handleAddBlock(const juce::var& json);
    void handleRemoveBlock(const juce::var& json);
    void handleMoveBlock(const juce::var& json);
    void handleAddConnection(const juce::var& json);
    void handleRemoveConnection(const juce::var& json);

    void emitToJs(const juce::String& eventName, juce::DynamicObject* detail);

    juce::WebBrowserComponent* webView = nullptr;
    StellarrProcessor* processor = nullptr;

    // Block UUID → graph node ID
    std::map<juce::String, juce::AudioProcessorGraph::NodeID> blockNodeMap;
    // Block UUID → grid position (col, row)
    std::map<juce::String, std::pair<int, int>> blockPositions;
};
