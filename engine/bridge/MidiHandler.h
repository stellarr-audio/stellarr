#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"

class StellarrProcessor;
namespace stellarr { class PluginBlock; }

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the pattern
    // established by UpdateHandler / ParamHandler / GraphHandler.
    //
    // The MidiMapper callbacks fire on the message thread (drained from
    // mapper.drainOutboundEvents() called by the editor's timer). Every
    // callback that mutates per-preset state early-returns when an async
    // restore is in flight, so the context exposes an `isRestoring` query
    // rather than reaching back into StellarrBridge::pendingRestore.
    //
    // Cross-handler reach (mark a plugin block dirty, mirror block state into
    // the active scene, drive preset/scene recall from CC mappings) is wrapped
    // as std::function callbacks rather than peer-handler references — keeps
    // MidiHandler unaware of ParamHandler / preset / scene class types and the
    // wider bridge state.
    struct MidiHandlerContext
    {
        StellarrProcessor& processor;
        const BlockNodeMap& blockNodeMap;
        IBridgeEmitter& emit;

        // Returns true while an async preset restore is in flight. Every
        // MidiMapper callback that mutates per-preset state must check this
        // and skip the mutation — Phase 2 of restoreSession will clearGraph()
        // and reload everything from the new preset, silently losing any
        // edit applied mid-load.
        std::function<bool()> isRestoring;

        // Mark a plugin block dirty and re-emit its state snapshot. Bridges
        // the bypass / mix / balance / level CC callbacks into ParamHandler
        // without a direct class coupling.
        std::function<void(const juce::String& blockId)> markBlockDirty;

        // Re-emit the full block parameter snapshot (mix / balance / level /
        // bypass / bypass mode) and the block-state snapshot. Used by the
        // onBlockState callback after a successful PluginBlock::recallState.
        std::function<void(const juce::String& blockId,
                           stellarr::PluginBlock* pluginBlock)> emitBlockParams;
        std::function<void(const juce::String& blockId,
                           stellarr::PluginBlock* pluginBlock)> emitBlockStates;

        // Mirror a block's new active-state index into the active scene's
        // blockStateMap so the rewire-dot prediction in the scene dropdown
        // reflects the new state, and re-emit scenes. Mirrors the post-recall
        // bookkeeping in handleBlockStateEvent("recall").
        std::function<void(const juce::String& blockId, int newActiveIndex)>
            mirrorActiveStateInScene;

        // Toggle the tuner active flag on every InputBlock (enable analysis)
        // and every OutputBlock (mute output) in the graph. Used by the
        // onTunerToggle CC callback.
        std::function<void(bool enabled)> setTunerActiveOnAllBlocks;

        // Switch the active preset to the one at `index`. Used by the
        // onPresetChange CC callback. Invokes the bridge's existing
        // handleLoadPresetByIndex flow which is itself try-locked, so it
        // self-rejects during a competing restore.
        std::function<void(int index)> loadPresetByIndex;

        // Recall the scene at `index` on the active preset. Used by the
        // onSceneSwitch CC callback.
        std::function<void(int index)> recallSceneByIndex;

        // Persist the current MidiMapper snapshot's global mappings
        // (preset change + tuner toggle) to ApplicationProperties. Used
        // by handleAddMidiMapping / handleRemoveMidiMapping /
        // handleClearMidiMappings so a global mapping survives a quit
        // even without a preset save.
        std::function<void()> persistGlobalMappings;
    };

    // Bridge handlers for MIDI mapping CRUD, learn, monitor, and the one-shot
    // initialisation that registers callbacks on MidiMapper. Constructing the
    // class registers the MidiMapper callbacks; destroying it clears them, so
    // captured-by-value context callbacks cannot fire after the handler dies.
    class MidiHandler
    {
    public:
        explicit MidiHandler(MidiHandlerContext ctx);
        ~MidiHandler();

        // Emit the current mapping list as a midiMappingsChanged event. Public
        // because GraphHandler's removeBlock path (and PresetHandler /
        // SessionSerializer in later commits) need to broadcast the new list
        // after pruning.
        void emitMidiMappings();

        // Dispatch handlers — called from the StellarrBridge dispatch table.
        void handleAddMidiMapping(const juce::var& json);
        void handleRemoveMidiMapping(const juce::var& json);
        void handleClearMidiMappings();
        void handleStartMidiLearn(const juce::var& json);
        void handleCancelMidiLearn();
        void handleSetMidiMonitorEnabled(const juce::var& json);
        void handleInjectMidiCC(const juce::var& json);
        void handleInjectMidiPC(const juce::var& json);

    private:
        void registerMapperCallbacks();
        void clearMapperCallbacks();

        MidiHandlerContext ctx;
    };
}
