#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include <vector>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"
#include "Scene.h"

class StellarrProcessor;
namespace stellarr { class PluginBlock; }

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the pattern
    // established by UpdateHandler / ParamHandler / GraphHandler / MidiHandler.
    //
    // SceneHandler now owns scenes + activeSceneIndex (Phase 7 / Commit 6).
    // Cross-handler reach (re-emitting per-block state + param snapshots after
    // a recall) is wrapped as std::function callbacks rather than peer-handler
    // references — keeps SceneHandler unaware of ParamHandler's class type.
    struct SceneHandlerContext
    {
        StellarrProcessor& processor;
        const BlockNodeMap& blockNodeMap;
        IBridgeEmitter& emit;

        // After a scene recall, every block whose binary state changed needs
        // its parameter + state snapshots rebroadcast to the UI so the
        // selected-state highlight, mix/balance/level fields and dirty dots
        // line up with the recalled values. Both fire on the message thread
        // from inside handleRecallScene.
        std::function<void(const juce::String& blockId,
                           stellarr::PluginBlock* pluginBlock)> emitBlockParams;
        std::function<void(const juce::String& blockId,
                           stellarr::PluginBlock* pluginBlock)> emitBlockStates;
    };

    // Bridge handlers for scene CRUD (add / recall / save / rename / delete)
    // plus the cross-handler API other handlers reach for via their context
    // callbacks (mirror block state into the active scene, drive scene recall
    // from a CC mapping, shift scene state-map entries when a block state is
    // deleted). Owns the scene list and the active-scene index — call sites
    // outside SceneHandler reach the data via the public accessors on this
    // class, never directly on StellarrBridge.
    class SceneHandler
    {
    public:
        explicit SceneHandler(SceneHandlerContext ctx);

        // Dispatch handlers — called from the StellarrBridge dispatch table.
        void handleAddScene();
        void handleRecallScene(const juce::var& json);
        void handleSaveScene(const juce::var& json);
        void handleRenameScene(const juce::var& json);
        void handleDeleteScene(const juce::var& json);

        // Broadcast the scenes list as a scenesChanged event. Public because
        // sendGraphState (still on StellarrBridge) emits scenes alongside the
        // graph snapshot, and SessionSerializer / PresetHandler trigger it
        // after restoring or seeding scenes.
        void emitScenes();

        // Mirror a block's new active-state index into the active scene's
        // blockStateMap so the rewire-dot prediction in the scene dropdown
        // reflects the change, then re-emit scenes. Called from ParamHandler
        // (UI-driven state change) and MidiHandler (CC-driven state change).
        // No-op when there is no active scene.
        void mirrorActiveStateInScene(const juce::String& blockId, int newActiveIndex);

        // Adjust every scene's blockStateMap entry for `blockId` after the
        // block state at `deletedIndex` was removed. Mirrors the index shift
        // that PluginBlock::deleteState() applied to its own active index.
        // `newCount` is the block's state count after deletion; pass 0 if the
        // block no longer exists. Called from ParamHandler.
        void shiftStateMappingsAfterDelete(const juce::String& blockId,
                                           int deletedIndex,
                                           int newCount);

        // Drive scene recall from a CC mapping. Returns true if `index` was
        // a valid scene; false if it was out of range. Called from MidiHandler.
        bool tryRecallSceneByIndex(int index);

        // Capture the current bridge state into the active scene if one is
        // active. No-op otherwise. Used by SessionSerializer before
        // serialising so unsaved tweaks made while the active scene was
        // current are persisted.
        void captureActiveScene();

        // Session serialise / restore + cross-handler read access.
        const std::vector<Scene>& getScenes() const { return scenes; }
        int getActiveSceneIndex() const { return activeSceneIndex; }
        void setScenes(std::vector<Scene> s) { scenes = std::move(s); }
        void setActiveSceneIndex(int idx) { activeSceneIndex = idx; }

    private:
        SceneHandlerContext ctx;
        std::vector<Scene> scenes;
        int activeSceneIndex = -1;
        static constexpr int maxScenes = 16;
    };
}
