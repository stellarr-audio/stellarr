#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <map>
#include <vector>

class StellarrProcessor;
namespace stellarr { class Block; class PluginBlock; }

class StellarrBridge
{
public:
    StellarrBridge();

    void setProcessor(StellarrProcessor* proc);
    void setAppProperties(juce::ApplicationProperties* props);
    juce::WebBrowserComponent::Options configureOptions(juce::WebBrowserComponent::Options options);
    void setWebView(juce::WebBrowserComponent* browser);

    struct Scene {
        juce::String name;
        std::map<juce::String, int> blockStateMap;
        std::map<juce::String, bool> blockBypassMap;
    };

    juce::var serialiseSession() const;
    void restoreSession(const juce::var& session);
    void sendSystemStats(double cpuPercent, double memoryMB, double totalMemoryMB);
    void sendTunerData();
    void sendMidiMonitorData();
    bool isTunerActive() const { return tunerActive; }
    void setOnUiReady(std::function<void()> callback) { onUiReady = std::move(callback); }

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
    void handleNewSession();
    void handleSaveSession();
    void handleSaveSessionQuiet();
    void handleLoadSession();
    void handlePickPresetDirectory();
    void handleLoadPresetByIndex(const juce::var& json);
    void handleRenamePreset(const juce::var& json);
    void handleDeletePreset(const juce::var& json);
    void handleGetPresetList();

    // MIDI mapping
    void setupMidiMapper();
    void emitMidiMappings();

    // Scene management
    void handleAddScene();
    void handleRecallScene(const juce::var& json);
    void handleSaveScene(const juce::var& json);
    void handleRenameScene(const juce::var& json);
    void handleDeleteScene(const juce::var& json);
    void emitScenes();

    void sendPluginList();
    void sendScanDirectories();
    void sendPresetList();

    void clearGraph();
    void setPresetFromFile(const juce::File& file);
    void persistPresetInfo();

    void emitToJs(const juce::String& eventName, juce::DynamicObject* detail);
    void emitBlockStates(const juce::String& blockId, stellarr::PluginBlock* pluginBlock);
    void emitBlockParams(const juce::String& blockId, stellarr::Block* block);
    void clearAllDirtyStates();

    // Block lookup helpers — return nullptr if not found
    stellarr::Block* findBlock(const juce::String& blockId);
    stellarr::PluginBlock* findPluginBlock(const juce::String& blockId);

    // Mark plugin block dirty and emit state update
    void markDirtyAndEmit(const juce::String& blockId, stellarr::Block* block);

    // Generic parameter handler (DRY)
    void handleSetBlockParam(const juce::var& json,
                              const juce::String& paramName,
                              std::function<void(stellarr::Block*, const juce::var&)> setter,
                              const juce::String& eventName,
                              std::function<juce::var(stellarr::Block*)> getter);

    // Generic block state handler
    void handleBlockStateEvent(const juce::var& json, const juce::String& action);

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
    std::function<void()> onUiReady;
    bool tunerActive = false;

    std::vector<Scene> scenes;
    int activeSceneIndex = -1;
    static constexpr int maxScenes = 16;
};
