#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"
#include "Scene.h"

class StellarrProcessor;

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the pattern
    // established by UpdateHandler / ParamHandler / GraphHandler / MidiHandler /
    // SceneHandler / InputBlockHandler.
    //
    // PresetHandler now owns presetDirectory / presetFiles / currentPresetIndex /
    // lastPresetFile / gridCols / gridRows (Phase 7 / Commit 8). appProperties
    // remains on StellarrBridge (set via setAppProperties) and is reached here
    // through a reference to that pointer slot, so the handler picks up a
    // late-binding setAppProperties call without re-emplace.
    //
    // Cross-cutting reach (graph add for the default IO seed, scene seeding,
    // dirty-state clear after save, MIDI mapping rebroadcast) is wrapped as
    // std::function callbacks rather than peer-handler references so
    // PresetHandler stays unaware of GraphHandler / SceneHandler / ParamHandler /
    // MidiHandler types.
    struct PresetHandlerContext
    {
        StellarrProcessor& processor;
        const BlockNodeMap& blockNodeMap;
        // Reference to the pointer slot on StellarrBridge so a late-arriving
        // setAppProperties() is observed without re-emplacing the handler.
        juce::ApplicationProperties*& appProperties;
        IBridgeEmitter& emit;

        // Bridge-level operations. clearGraph + serialiseSession + restoreSession
        // still live on StellarrBridge until the SessionSerializer extraction
        // (Commit 9); routing them through std::function keeps PresetHandler
        // independent of those decisions.
        std::function<void()>                                       clearGraph;
        std::function<juce::var()>                                  serialiseSession;
        std::function<bool(juce::var, std::function<void(bool)>)>   restoreSession;
        std::function<void()>                                       sendGraphState;

        // Cross-handler operations. handleNewSession seeds default input/output
        // blocks via GraphHandler::handleAddBlock, hands a freshly-captured
        // default scene to SceneHandler, and re-emits MIDI mappings after the
        // preset-level mapping list is cleared. handleSaveSession +
        // handleSaveSessionQuiet clear the dirty-state set after a successful
        // write so the dirty-dot UI matches the on-disk session.
        std::function<void(const juce::var&)>                       graphAddBlock;
        std::function<void(std::vector<Scene>)>                     setScenes;
        std::function<void(int)>                                    setActiveSceneIndex;
        std::function<void()>                                       clearAllDirtyStates;
        std::function<void()>                                       emitMidiMappings;
    };

    // Bridge handlers for preset / session CRUD plus grid sizing. Owns the
    // preset directory + file list + active-preset bookkeeping + grid
    // dimensions; cross-handler call sites (SessionSerializer, the bridge
    // start-up flow) read those via the public accessors on this class rather
    // than reaching into StellarrBridge.
    class PresetHandler
    {
    public:
        explicit PresetHandler(PresetHandlerContext ctx);

        // Dispatch handlers — called from the StellarrBridge dispatch table.
        void handleNewSession();
        void handleSaveSession();
        void handleSaveSessionQuiet();
        void handleLoadSession();
        void handlePickPresetDirectory();
        void handleLoadPresetByIndex(const juce::var& json);
        void handleRenamePreset(const juce::var& json);
        void handleDeletePreset(const juce::var& json);
        void handleGetPresetList();
        void handleSetGridSize(const juce::var& json);

        // Cross-handler API. emitGridState + sendPresetList are called by the
        // bridge start-up flow + SessionSerializer::finishRestore;
        // setPresetFromFile is the post-restore bookkeeping path used by both
        // the UI-driven save / load and the start-up last-session restore;
        // persistPresetInfo is also used by start-up to write the global MIDI
        // mapping snapshot.
        void emitGridState();
        void sendPresetList();
        void setPresetFromFile(const juce::File& file);
        void persistPresetInfo();

        // Test accessors / cross-handler reads.
        const juce::StringArray& getPresetFiles() const { return presetFiles; }
        int getCurrentPresetIndex() const { return currentPresetIndex; }
        const juce::File& getLastPresetFile() const { return lastPresetFile; }
        const juce::File& getPresetDirectory() const { return presetDirectory; }

        // Setters used by the bridge start-up flow + SessionSerializer when
        // restoring a session. Plain setters rather than full bookkeeping
        // helpers because the start-up path interleaves these with directory
        // scans + currentPresetIndex updates.
        void setPresetDirectory(const juce::File& dir) { presetDirectory = dir; }
        void setLastPresetFile(const juce::File& file) { lastPresetFile = file; }
        void setCurrentPresetIndex(int idx) { currentPresetIndex = idx; }

        int getGridCols() const { return gridCols; }
        int getGridRows() const { return gridRows; }
        void setGridCols(int c) { gridCols = c; }
        void setGridRows(int r) { gridRows = r; }

    private:
        PresetHandlerContext ctx;

        juce::File presetDirectory;
        juce::StringArray presetFiles;
        int currentPresetIndex = -1;
        juce::File lastPresetFile;

        // Grid size — persisted with the session. Defaults match the UI.
        int gridCols = 12;
        int gridRows = 5;
    };
}
