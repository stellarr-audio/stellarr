#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"

class StellarrProcessor;
namespace stellarr { class Block; class PluginBlock; }

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the pattern
    // introduced by UpdateHandler in Phase 7 / Commit 2.
    //
    // The two callbacks let handleBlockStateEvent("delete" / "recall" / ...)
    // hook into StellarrBridge's scene + MIDI mapping bookkeeping without
    // reaching back into the bridge's full type. Both fire on the message
    // thread from inside handleBlockStateEvent.
    struct ParamHandlerContext
    {
        StellarrProcessor& processor;
        const BlockNodeMap& blockNodeMap;
        IBridgeEmitter& emit;

        // Called after a successful PluginBlock::deleteState(index). The
        // handler is responsible for shifting any per-scene state map and any
        // MIDI blockState mappings whose targetIndex sat at or after `index`,
        // and re-emitting midiMappings.
        std::function<void(const juce::String& blockId, int deletedIndex)> onBlockStateDeleted;

        // Called whenever the active state index for `blockId` shifts as a
        // result of a UI-driven event (add / recall / delete). The handler
        // mirrors the new active index into the active scene's blockStateMap
        // and re-emits scenes if appropriate.
        std::function<void(const juce::String& blockId, int newActiveIndex)> onActiveStateChanged;
    };

    // Bridge handlers for per-block parameter mutations (mix / balance / level
    // / bypass mode), block state save / add / recall / delete events, and the
    // emit helpers that broadcast block parameter + state snapshots back to
    // the UI. The emit + markDirtyAndEmit helpers are public so other (still
    // free-function) handlers — SceneHandler, MidiHandler, GraphHandler,
    // PresetHandler — can route through them via StellarrBridge::param while
    // the Phase 7 refactor is in flight.
    class ParamHandler
    {
    public:
        explicit ParamHandler(ParamHandlerContext ctx);

        void handleSetBlockMix(const juce::var& json);
        void handleSetBlockBalance(const juce::var& json);
        void handleSetBlockLevel(const juce::var& json);
        void handleSetBlockBypassMode(const juce::var& json);
        void handleBlockStateEvent(const juce::var& json, const juce::String& action);

        // Emit helpers — public so the still-free-function handlers can call
        // them via StellarrBridge::param->... until they too are wrapped in
        // their own classes in later Phase 7 commits.
        void emitBlockStates(const juce::String& blockId, stellarr::PluginBlock* pluginBlock);
        void emitBlockParams(const juce::String& blockId, stellarr::Block* block);
        void clearAllDirtyStates();

        // Mark plugin block dirty and re-emit its state snapshot. Public for
        // the same reason as the emit helpers above — MidiHandler and
        // GraphHandler still call it directly.
        void markDirtyAndEmit(const juce::String& blockId);

    private:
        // Generic primitive backing the four setBlock<X> handlers. `setter`
        // applies the new value, `getter` reads the post-set value back for
        // the change-event payload.
        void handleSetBlockParam(const juce::var& json,
                                 const juce::String& paramName,
                                 std::function<void(stellarr::Block*, const juce::var&)> setter,
                                 const juce::String& eventName,
                                 std::function<juce::var(stellarr::Block*)> getter);

        ParamHandlerContext ctx;
    };
}
