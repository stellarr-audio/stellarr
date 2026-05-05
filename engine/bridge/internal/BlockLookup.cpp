#include "BlockLookup.h"
#include "../../StellarrProcessor.h"
#include "../../blocks/Block.h"
#include "../../blocks/PluginBlock.h"

namespace stellarr::bridge::internal
{
    juce::AudioProcessorGraph::Node* getNodeForBlockId(
        const BlockNodeMap& blockNodeMap,
        StellarrProcessor& processor,
        const juce::String& blockId)
    {
        auto it = blockNodeMap.find(blockId);
        if (it == blockNodeMap.end()) return nullptr;
        return processor.getGraph().getNodeForId(it->second);
    }

    stellarr::Block* findBlock(
        const BlockNodeMap& blockNodeMap,
        StellarrProcessor& processor,
        const juce::String& blockId)
    {
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return nullptr;

        auto* node = processor.getGraph().getNodeForId(nodeIt->second);
        if (node == nullptr) return nullptr;

        return dynamic_cast<stellarr::Block*>(node->getProcessor());
    }

    stellarr::PluginBlock* findPluginBlock(
        const BlockNodeMap& blockNodeMap,
        StellarrProcessor& processor,
        const juce::String& blockId)
    {
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return nullptr;

        auto* node = processor.getGraph().getNodeForId(nodeIt->second);
        if (node == nullptr) return nullptr;

        return dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
    }
}
