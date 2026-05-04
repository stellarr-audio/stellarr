#pragma once
#include "../BridgeTypes.h"

class StellarrProcessor;
namespace stellarr { class Block; class PluginBlock; }

namespace stellarr::bridge::internal
{
    // Block / node lookup helpers shared across multiple bridge handlers.
    // Pure functions; no state.
    //
    // Arg ordering follows the bridge convention: data-source args precede
    // query args (BlockNodeMap, processor, blockId).

    juce::AudioProcessorGraph::Node* getNodeForBlockId(
        const BlockNodeMap& blockNodeMap,
        StellarrProcessor& processor,
        const juce::String& blockId);

    stellarr::Block* findBlock(
        const BlockNodeMap& blockNodeMap,
        StellarrProcessor& processor,
        const juce::String& blockId);

    stellarr::PluginBlock* findPluginBlock(
        const BlockNodeMap& blockNodeMap,
        StellarrProcessor& processor,
        const juce::String& blockId);
}
