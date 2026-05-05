#pragma once
#include <functional>
#include <juce_core/juce_core.h>
#include <map>
#include <utility>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"

class StellarrProcessor;

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the Phase 7 pattern
    // established by UpdateHandler / ParamHandler.
    //
    // Cross-cutting state mutations (mark a plugin block dirty, re-emit MIDI
    // mappings, broadcast a full graph snapshot) are wrapped as std::function
    // callbacks rather than peer-handler references — keeps GraphHandler
    // unaware of ParamHandler / MidiHandler / preset-state class types.
    struct GraphHandlerContext
    {
        StellarrProcessor& processor;
        BlockNodeMap& blockNodeMap;                                      // non-const: Graph mutates the map
        std::map<juce::String, std::pair<int, int>>& blockPositions;     // non-const: positions move with blocks
        juce::var& clipboardJson;                                        // non-const: copy/paste read + write
        IBridgeEmitter& emit;

        // Mark a plugin block dirty and re-emit its state snapshot. Bridges
        // GraphHandler::handleToggleBlockBypass into ParamHandler without a
        // direct class coupling.
        std::function<void(const juce::String& blockId)> markBlockDirty;

        // Re-emit MIDI mappings to the UI. Used by handleRemoveBlock after a
        // block is deleted and any mappings targeting it have been pruned.
        std::function<void()> emitMidiMappings;

        // Broadcast a full graph snapshot to the UI. Used by handlePasteBlock
        // since a freshly-pasted block needs every property (mix, level,
        // plugin info, ...) sent — the simple blockAdded event only carries
        // the basics.
        std::function<void()> sendGraphState;
    };

    // Bridge handlers for block + connection CRUD, clipboard copy/paste,
    // plugin editor open, and per-block metadata (rename / colour / bypass).
    class GraphHandler
    {
    public:
        explicit GraphHandler(GraphHandlerContext ctx);

        // Block CRUD
        void handleAddBlock(const juce::var& json);
        void handleRemoveBlock(const juce::var& json);
        void handleMoveBlock(const juce::var& json);

        // Connection CRUD
        void handleAddConnection(const juce::var& json);
        void handleRemoveConnection(const juce::var& json);

        // Plugin attach + editor
        void handleSetBlockPlugin(const juce::var& json);
        void handleOpenPluginEditor(const juce::var& json);

        // Clipboard
        void handleCopyBlock(const juce::var& json);
        void handlePasteBlock(const juce::var& json);

        // Block metadata
        void handleRenameBlock(const juce::var& json);
        void handleSetBlockColor(const juce::var& json);
        void handleToggleBlockBypass(const juce::var& json);

    private:
        GraphHandlerContext ctx;
    };
}
