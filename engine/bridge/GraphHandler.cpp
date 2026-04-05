#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include "../blocks/PluginBlock.h"

void StellarrBridge::handleAddBlock(const juce::var& json)
{
    if (processor == nullptr) return;

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
    auto nodeId = processor->addBlock(std::move(block));

    if (nodeId.uid == 0) return;

    blockNodeMap[blockId] = nodeId;
    blockPositions[blockId] = {col, row};

    if (type == "input")
    {
        processor->disconnectBlocks(processor->getAudioInputNodeId(), processor->getAudioOutputNodeId());
        processor->connectBlocks(processor->getAudioInputNodeId(), nodeId);
        // Connect MIDI system input to this input block
        processor->getGraph().addConnection({
            {processor->getMidiInputNodeId(), juce::AudioProcessorGraph::midiChannelIndex},
            {nodeId, juce::AudioProcessorGraph::midiChannelIndex}
        });
    }
    else if (type == "output")
    {
        processor->disconnectBlocks(processor->getAudioInputNodeId(), processor->getAudioOutputNodeId());
        processor->connectBlocks(nodeId, processor->getAudioOutputNodeId());
        // Connect this output block to MIDI system output
        processor->getGraph().addConnection({
            {nodeId, juce::AudioProcessorGraph::midiChannelIndex},
            {processor->getMidiOutputNodeId(), juce::AudioProcessorGraph::midiChannelIndex}
        });
    }

    // Splice: insert the new block into an existing connection
    auto spliceSourceId = obj->getProperty("spliceSourceId").toString();
    auto spliceDestId   = obj->getProperty("spliceDestId").toString();

    if (spliceSourceId.isNotEmpty() && spliceDestId.isNotEmpty())
    {
        auto srcIt = blockNodeMap.find(spliceSourceId);
        auto dstIt = blockNodeMap.find(spliceDestId);

        if (srcIt != blockNodeMap.end() && dstIt != blockNodeMap.end())
        {
            processor->disconnectBlocks(srcIt->second, dstIt->second);
            processor->connectBlocks(srcIt->second, nodeId);
            processor->connectBlocks(nodeId, dstIt->second);

            auto* connDetail1 = new juce::DynamicObject();
            connDetail1->setProperty("sourceId", spliceSourceId);
            connDetail1->setProperty("destId", blockId);
            emitToJs("connectionAdded", connDetail1);

            auto* connDetail2 = new juce::DynamicObject();
            connDetail2->setProperty("sourceId", blockId);
            connDetail2->setProperty("destId", spliceDestId);
            emitToJs("connectionAdded", connDetail2);

            auto* connRemoved = new juce::DynamicObject();
            connRemoved->setProperty("sourceId", spliceSourceId);
            connRemoved->setProperty("destId", spliceDestId);
            emitToJs("connectionRemoved", connRemoved);
        }
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("id", blockId);
    detail->setProperty("type", type);
    detail->setProperty("name", blockName);
    detail->setProperty("col", col);
    detail->setProperty("row", row);
    detail->setProperty("nodeId", static_cast<int>(nodeId.uid));
    emitToJs("blockAdded", detail);
}

void StellarrBridge::handleRemoveBlock(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto it = blockNodeMap.find(blockId);
    if (it == blockNodeMap.end()) return;

    processor->removeBlock(it->second);
    blockNodeMap.erase(it);
    blockPositions.erase(blockId);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    emitToJs("blockRemoved", detail);
}

void StellarrBridge::handleMoveBlock(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto col = static_cast<int>(obj->getProperty("col"));
    auto row = static_cast<int>(obj->getProperty("row"));

    blockPositions[blockId] = {col, row};

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("col", col);
    detail->setProperty("row", row);
    emitToJs("blockMoved", detail);
}

void StellarrBridge::handleAddConnection(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto sourceId = obj->getProperty("sourceId").toString();
    auto destId   = obj->getProperty("destId").toString();

    auto srcIt = blockNodeMap.find(sourceId);
    auto dstIt = blockNodeMap.find(destId);
    if (srcIt == blockNodeMap.end() || dstIt == blockNodeMap.end()) return;

    if (!processor->connectBlocks(srcIt->second, dstIt->second))
        return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("sourceId", sourceId);
    detail->setProperty("destId", destId);
    emitToJs("connectionAdded", detail);
}

void StellarrBridge::handleRemoveConnection(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto sourceId = obj->getProperty("sourceId").toString();
    auto destId   = obj->getProperty("destId").toString();

    auto srcIt = blockNodeMap.find(sourceId);
    auto dstIt = blockNodeMap.find(destId);
    if (srcIt == blockNodeMap.end() || dstIt == blockNodeMap.end()) return;

    processor->disconnectBlocks(srcIt->second, dstIt->second);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("sourceId", sourceId);
    detail->setProperty("destId", destId);
    emitToJs("connectionRemoved", detail);
}

void StellarrBridge::handleSetBlockPlugin(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId  = obj->getProperty("blockId").toString();
    auto pluginId = obj->getProperty("pluginId").toString();

    auto* pluginBlock = findPluginBlock(blockId);
    if (pluginBlock == nullptr) return;

    juce::String errorMessage;
    auto instance = processor->getPluginManager().createPluginInstance(
        pluginId, processor->getSampleRate(),
        processor->getBlockSize(), errorMessage);

    if (instance == nullptr) return;

    auto pluginName = instance->getName();
    pluginBlock->setPlugin(std::move(instance), pluginId);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("pluginId", pluginId);
    detail->setProperty("pluginName", pluginName);
    detail->setProperty("pluginFormat", pluginBlock->getPluginFormat());
    detail->setProperty("hasEditor", true);
    emitToJs("blockPluginSet", detail);
}

void StellarrBridge::handleCopyBlock(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    auto* node = processor->getGraph().getNodeForId(nodeIt->second);
    if (node == nullptr) return;

    auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor());
    if (block == nullptr) return;

    clipboardJson = block->toJson();

    auto* detail = new juce::DynamicObject();
    detail->setProperty("type", stellarr::blockTypeToString(block->getBlockType()));
    emitToJs("blockCopied", detail);
}

void StellarrBridge::handlePasteBlock(const juce::var& json)
{
    if (processor == nullptr) return;
    if (!clipboardJson.getDynamicObject()) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto col = static_cast<int>(obj->getProperty("col"));
    auto row = static_cast<int>(obj->getProperty("row"));

    auto* clipObj = clipboardJson.getDynamicObject();
    auto type = clipObj->getProperty("type").toString();

    std::unique_ptr<stellarr::Block> block;
    if (type == "input")       block = std::make_unique<stellarr::InputBlock>();
    else if (type == "output") block = std::make_unique<stellarr::OutputBlock>();
    else if (type == "plugin" || type == "vst") block = std::make_unique<stellarr::PluginBlock>();
    else return;

    block->fromJson(clipboardJson);
    block->regenerateBlockId();
    block->resetToDefault();

    auto blockId = block->getBlockId().toString();
    auto blockName = block->getName();
    auto nodeId = processor->addBlock(std::move(block));
    if (nodeId.uid == 0) return;

    blockNodeMap[blockId] = nodeId;
    blockPositions[blockId] = {col, row};

    if (type == "input")
    {
        processor->connectBlocks(processor->getAudioInputNodeId(), nodeId);
        processor->getGraph().addConnection({
            {processor->getMidiInputNodeId(), juce::AudioProcessorGraph::midiChannelIndex},
            {nodeId, juce::AudioProcessorGraph::midiChannelIndex}
        });
    }
    else if (type == "output")
    {
        processor->connectBlocks(nodeId, processor->getAudioOutputNodeId());
        processor->getGraph().addConnection({
            {nodeId, juce::AudioProcessorGraph::midiChannelIndex},
            {processor->getMidiOutputNodeId(), juce::AudioProcessorGraph::midiChannelIndex}
        });
    }

    // Restore plugin instance for plugin blocks
    if (type == "plugin" || type == "vst")
    {
        auto pluginId = clipObj->getProperty("pluginId").toString();
        if (pluginId.isNotEmpty())
        {
            auto* node = processor->getGraph().getNodeForId(nodeId);
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                juce::String errorMessage;
                auto instance = processor->getPluginManager().createPluginInstance(
                    pluginId, processor->getSampleRate(),
                    processor->getBlockSize(), errorMessage);

                if (instance != nullptr)
                {
                    pluginBlock->setPlugin(std::move(instance), pluginId);
                    pluginBlock->restorePluginState();
                }
            }
        }
    }

    // Full graph sync to ensure all block properties (mix, level, plugin info, etc.)
    // are sent to the UI — a simple blockAdded event only carries basic fields.
    sendGraphState();
}

void StellarrBridge::handleOpenPluginEditor(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* pluginBlock = findPluginBlock(blockId);
    if (pluginBlock == nullptr || !pluginBlock->hasPlugin()) return;

    juce::MessageManager::callAsync([pluginBlock]()
    {
        pluginBlock->openPluginEditor();
    });
}
