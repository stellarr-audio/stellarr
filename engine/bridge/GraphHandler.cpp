#include "GraphHandler.h"
#include "../StellarrProcessor.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include "../blocks/PluginBlock.h"
#include "EventNames.h"
#include "internal/BlockLookup.h"
#include "internal/BlockLifecycle.h"

namespace stellarr::bridge {

using namespace stellarr::bridge::internal;

GraphHandler::GraphHandler(GraphHandlerContext c) : ctx(c) {}

void GraphHandler::handleAddBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto type = obj->getProperty("type").toString();
    auto col  = static_cast<int>(obj->getProperty("col"));
    auto row  = static_cast<int>(obj->getProperty("row"));

    std::unique_ptr<stellarr::Block> block;

    if (type == "input")       block = std::make_unique<stellarr::InputBlock>();
    else if (type == "output") block = std::make_unique<stellarr::OutputBlock>();
    else if (type == "plugin") block = std::make_unique<stellarr::PluginBlock>();
    else return;

    auto blockId = block->getBlockId().toString();
    auto blockName = block->getName();

    auto spliceSourceId = obj->getProperty("spliceSourceId").toString();
    auto spliceDestId   = obj->getProperty("spliceDestId").toString();

    juce::AudioProcessorGraph::NodeID nodeId;
    bool spliceApplied = false;

    {
        StellarrProcessor::GraphMutationScope scope(ctx.processor);
        using UK = StellarrProcessor::UpdateKind;

        nodeId = ctx.processor.addBlock(std::move(block), UK::none);
        if (nodeId.uid == 0) return;

        ctx.blockNodeMap[blockId] = nodeId;
        ctx.blockPositions[blockId] = {col, row};

        if (type == "input" || type == "output")
        {
            // connectIOBlock disconnects the default audioInput -> audioOutput
            // bypass before wiring; no separate disconnect needed here.
            connectIOBlock(ctx.processor, type, nodeId, UK::none);
        }

        // Splice: insert the new block into an existing connection
        if (spliceSourceId.isNotEmpty() && spliceDestId.isNotEmpty())
        {
            auto srcIt = ctx.blockNodeMap.find(spliceSourceId);
            auto dstIt = ctx.blockNodeMap.find(spliceDestId);

            if (srcIt != ctx.blockNodeMap.end() && dstIt != ctx.blockNodeMap.end())
            {
                ctx.processor.disconnectBlocks(srcIt->second, dstIt->second, UK::none);
                ctx.processor.connectBlocks(srcIt->second, nodeId, 2, UK::none);
                ctx.processor.connectBlocks(nodeId, dstIt->second, 2, UK::none);
                spliceApplied = true;
            }
        }
    }

    if (spliceApplied)
    {
        auto* connDetail1 = new juce::DynamicObject();
        connDetail1->setProperty("sourceId", spliceSourceId);
        connDetail1->setProperty("destId", blockId);
        ctx.emit.emit(events::ConnectionAdded, connDetail1);

        auto* connDetail2 = new juce::DynamicObject();
        connDetail2->setProperty("sourceId", blockId);
        connDetail2->setProperty("destId", spliceDestId);
        ctx.emit.emit(events::ConnectionAdded, connDetail2);

        auto* connRemoved = new juce::DynamicObject();
        connRemoved->setProperty("sourceId", spliceSourceId);
        connRemoved->setProperty("destId", spliceDestId);
        ctx.emit.emit(events::ConnectionRemoved, connRemoved);
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("id", blockId);
    detail->setProperty("type", type);
    detail->setProperty("name", blockName);
    detail->setProperty("col", col);
    detail->setProperty("row", row);
    detail->setProperty("nodeId", static_cast<int>(nodeId.uid));
    ctx.emit.emit(events::BlockAdded, detail);
}

void GraphHandler::handleRemoveBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto it = ctx.blockNodeMap.find(blockId);
    if (it == ctx.blockNodeMap.end()) return;

    ctx.processor.getMidiMapper().removeMappingsForBlock(it->first);
    ctx.emitMidiMappings();

    {
        StellarrProcessor::GraphMutationScope scope(ctx.processor);
        ctx.processor.removeBlock(it->second, StellarrProcessor::UpdateKind::none);
    }

    ctx.blockNodeMap.erase(it);
    ctx.blockPositions.erase(blockId);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    ctx.emit.emit(events::BlockRemoved, detail);
}

void GraphHandler::handleMoveBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto col = static_cast<int>(obj->getProperty("col"));
    auto row = static_cast<int>(obj->getProperty("row"));

    ctx.blockPositions[blockId] = {col, row};

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("col", col);
    detail->setProperty("row", row);
    ctx.emit.emit(events::BlockMoved, detail);
}

void GraphHandler::handleAddConnection(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto sourceId = obj->getProperty("sourceId").toString();
    auto destId   = obj->getProperty("destId").toString();

    auto srcIt = ctx.blockNodeMap.find(sourceId);
    auto dstIt = ctx.blockNodeMap.find(destId);
    if (srcIt == ctx.blockNodeMap.end() || dstIt == ctx.blockNodeMap.end()) return;

    bool ok = false;
    {
        StellarrProcessor::GraphMutationScope scope(ctx.processor);
        ok = ctx.processor.connectBlocks(srcIt->second, dstIt->second, 2,
                                          StellarrProcessor::UpdateKind::none);
    }

    if (!ok) return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("sourceId", sourceId);
    detail->setProperty("destId", destId);
    ctx.emit.emit(events::ConnectionAdded, detail);
}

void GraphHandler::handleRemoveConnection(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto sourceId = obj->getProperty("sourceId").toString();
    auto destId   = obj->getProperty("destId").toString();

    auto srcIt = ctx.blockNodeMap.find(sourceId);
    auto dstIt = ctx.blockNodeMap.find(destId);
    if (srcIt == ctx.blockNodeMap.end() || dstIt == ctx.blockNodeMap.end()) return;

    {
        StellarrProcessor::GraphMutationScope scope(ctx.processor);
        ctx.processor.disconnectBlocks(srcIt->second, dstIt->second,
                                        StellarrProcessor::UpdateKind::none);
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("sourceId", sourceId);
    detail->setProperty("destId", destId);
    ctx.emit.emit(events::ConnectionRemoved, detail);
}

void GraphHandler::handleSetBlockPlugin(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId  = obj->getProperty("blockId").toString();
    auto pluginId = obj->getProperty("pluginId").toString();

    auto* pluginBlock = findPluginBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (pluginBlock == nullptr) return;

    juce::String errorMessage;
    auto instance = ctx.processor.getPluginManager().createPluginInstance(
        pluginId, ctx.processor.getSampleRate(),
        ctx.processor.getBlockSize(), errorMessage);

    if (instance == nullptr) return;

    auto pluginName = instance->getName();

    // Live plugin-picker path: defer the block's "ready" flag for a few seconds
    // so plugins with async background initialisation (NAM, IR loaders, ML amp
    // sims) are not invoked until they have finished loading. While deferred,
    // the block is pass-through, so the rest of the chain keeps audio.
    pluginBlock->setPlugin(std::move(instance), pluginId,
                           stellarr::PluginBlock::pluginPickReadyDelayMs);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("pluginId", pluginId);
    detail->setProperty("pluginName", pluginName);
    detail->setProperty("pluginFormat", pluginBlock->getPluginFormat());
    detail->setProperty("hasEditor", true);
    detail->setProperty("pluginMissing", false);
    ctx.emit.emit(events::BlockPluginSet, detail);
}

void GraphHandler::handleCopyBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto nodeIt = ctx.blockNodeMap.find(blockId);
    if (nodeIt == ctx.blockNodeMap.end()) return;

    auto* node = ctx.processor.getGraph().getNodeForId(nodeIt->second);
    if (node == nullptr) return;

    auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor());
    if (block == nullptr) return;

    ctx.clipboardJson = block->toJson();

    auto* detail = new juce::DynamicObject();
    detail->setProperty("type", stellarr::blockTypeToString(block->getBlockType()));
    ctx.emit.emit(events::BlockCopied, detail);
}

void GraphHandler::handlePasteBlock(const juce::var& json)
{
    if (!ctx.clipboardJson.getDynamicObject()) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto col = static_cast<int>(obj->getProperty("col"));
    auto row = static_cast<int>(obj->getProperty("row"));

    auto* clipObj = ctx.clipboardJson.getDynamicObject();
    auto type = clipObj->getProperty("type").toString();

    std::unique_ptr<stellarr::Block> block;
    if (type == "input")       block = std::make_unique<stellarr::InputBlock>();
    else if (type == "output") block = std::make_unique<stellarr::OutputBlock>();
    else if (type == "plugin" || type == "vst") block = std::make_unique<stellarr::PluginBlock>();
    else return;

    block->fromJson(ctx.clipboardJson);
    block->regenerateBlockId();
    block->resetToDefault();

    auto blockId = block->getBlockId().toString();
    auto blockName = block->getName();
    auto nodeId = ctx.processor.addBlock(std::move(block));
    if (nodeId.uid == 0) return;

    ctx.blockNodeMap[blockId] = nodeId;
    ctx.blockPositions[blockId] = {col, row};

    connectIOBlock(ctx.processor, type, nodeId);

    if (type == "plugin" || type == "vst")
        restoreBlockPlugin(ctx.processor, nodeId, clipObj->getProperty("pluginId").toString(),
                           clipObj->getProperty("pluginName").toString());

    // Full graph sync to ensure all block properties (mix, level, plugin info, etc.)
    // are sent to the UI — a simple blockAdded event only carries basic fields.
    ctx.sendGraphState();
}

void GraphHandler::handleOpenPluginEditor(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* pluginBlock = findPluginBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (pluginBlock == nullptr || !pluginBlock->hasPlugin()) return;

    juce::MessageManager::callAsync([pluginBlock]()
    {
        pluginBlock->openPluginEditor();
    });
}

// -- Block metadata handlers --------------------------------------------------

void GraphHandler::handleRenameBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto name = obj->getProperty("name").toString();
    auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (block == nullptr) return;

    block->setDisplayName(name);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("displayName", name);
    ctx.emit.emit(events::BlockRenamed, detail);
}

void GraphHandler::handleSetBlockColor(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto color = obj->getProperty("color").toString();
    auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (block == nullptr) return;

    block->setBlockColor(color);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("blockColor", color);
    ctx.emit.emit(events::BlockColorChanged, detail);
}

void GraphHandler::handleToggleBlockBypass(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (block == nullptr) return;

    bool newState = !block->isBypassed();
    block->setBypassed(newState);
    ctx.markBlockDirty(blockId);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("bypassed", newState);
    ctx.emit.emit(events::BlockBypassChanged, detail);
}

} // namespace stellarr::bridge
