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
    void setAppProperties(juce::ApplicationProperties* props);
    juce::WebBrowserComponent::Options configureOptions(juce::WebBrowserComponent::Options options);
    void setWebView(juce::WebBrowserComponent* browser);

    juce::var serialiseSession() const;
    void restoreSession(const juce::var& session);
    void sendSystemStats(double cpuPercent, double memoryMB, double totalMemoryMB);

private:
    void handleEvent(const juce::String& eventName, const juce::var& payload);
    void handleBridgeReady();
    void sendStartupProgress(const juce::String& status, int progress);
    void sendWelcome();
    void sendGraphState();

    // Graph event handlers
    void handleAddBlock(const juce::var& json);
    void handleRemoveBlock(const juce::var& json);
    void handleMoveBlock(const juce::var& json);
    void handleAddConnection(const juce::var& json);
    void handleRemoveConnection(const juce::var& json);
    void handleSetBlockPlugin(const juce::var& json);
    void handleOpenPluginEditor(const juce::var& json);

    // Plugin management event handlers
    void handleScanPlugins();
    void handleGetScanDirectories();
    void handlePickScanDirectory();
    void handleRemoveScanDirectory(const juce::var& json);

    // Preset management
    void handleSaveSession();
    void handleLoadSession();
    void handlePickPresetDirectory();
    void handleLoadPresetByIndex(const juce::var& json);
    void handleGetPresetList();

    void sendPluginList();
    void sendScanDirectories();
    void sendPresetList();

    void clearGraph();
    void setPresetFromFile(const juce::File& file);
    void persistPresetInfo();

    void emitToJs(const juce::String& eventName, juce::DynamicObject* detail);

    juce::WebBrowserComponent* webView = nullptr;
    StellarrProcessor* processor = nullptr;
    juce::ApplicationProperties* appProperties = nullptr;

    std::map<juce::String, juce::AudioProcessorGraph::NodeID> blockNodeMap;
    std::map<juce::String, std::pair<int, int>> blockPositions;

    // Preset directory and browsing
    juce::File presetDirectory;
    juce::StringArray presetFiles;
    int currentPresetIndex = -1;
    juce::File lastPresetFile;
};
