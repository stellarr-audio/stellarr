#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../blocks/Block.h"
#include "../blocks/PluginBlock.h"

// -- Block parameter handlers -------------------------------------------------
// These all follow the same pattern: parse blockId + value, find block, set
// param, mark dirty if plugin block, emit change event.

void StellarrBridge::handleSetBlockParam(const juce::var& json,
                                          const juce::String& paramName,
                                          std::function<void(stellarr::Block*, const juce::var&)> setter,
                                          const juce::String& eventName,
                                          std::function<juce::var(stellarr::Block*)> getter)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* block = findBlock(blockId);
    if (block == nullptr) return;

    setter(block, obj->getProperty(paramName));
    markDirtyAndEmit(blockId, block);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty(paramName, getter(block));
    emitToJs(eventName, detail);
}

// -- Block state handlers (save/add/recall/delete) ----------------------------

void StellarrBridge::handleBlockStateEvent(const juce::var& json, const juce::String& action)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* pluginBlock = findPluginBlock(blockId);
    if (pluginBlock == nullptr) return;

    if (action == "save")
    {
        pluginBlock->saveCurrentState();
    }
    else if (action == "add")
    {
        pluginBlock->addState();
    }
    else if (action == "recall")
    {
        auto index = static_cast<int>(obj->getProperty("index"));
        if (pluginBlock->recallState(index))
            emitBlockParams(blockId, pluginBlock);
    }
    else if (action == "delete")
    {
        auto index = static_cast<int>(obj->getProperty("index"));
        if (pluginBlock->deleteState(index))
            emitBlockParams(blockId, pluginBlock);
    }

    emitBlockStates(blockId, pluginBlock);
}

// -- Emit helpers -------------------------------------------------------------

void StellarrBridge::emitBlockStates(const juce::String& blockId, stellarr::PluginBlock* pluginBlock)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("numStates", pluginBlock->getNumStates());
    detail->setProperty("activeStateIndex", pluginBlock->getActiveStateIndex());

    juce::Array<juce::var> dirtyArr;
    for (int d : pluginBlock->getDirtyStates())
        dirtyArr.add(d);
    detail->setProperty("dirtyStates", dirtyArr);

    emitToJs("blockStatesChanged", detail);
}

void StellarrBridge::emitBlockParams(const juce::String& blockId, stellarr::Block* block)
{
    auto* mixDetail = new juce::DynamicObject();
    mixDetail->setProperty("blockId", blockId);
    mixDetail->setProperty("mix", static_cast<double>(block->getMix()));
    emitToJs("blockMixChanged", mixDetail);

    auto* balDetail = new juce::DynamicObject();
    balDetail->setProperty("blockId", blockId);
    balDetail->setProperty("balance", static_cast<double>(block->getBalance()));
    emitToJs("blockBalanceChanged", balDetail);

    auto* lvlDetail = new juce::DynamicObject();
    lvlDetail->setProperty("blockId", blockId);
    lvlDetail->setProperty("level", static_cast<double>(block->getLevelDb()));
    emitToJs("blockLevelChanged", lvlDetail);

    auto* bypDetail = new juce::DynamicObject();
    bypDetail->setProperty("blockId", blockId);
    bypDetail->setProperty("bypassed", block->isBypassed());
    emitToJs("blockBypassChanged", bypDetail);

    auto* modeDetail = new juce::DynamicObject();
    modeDetail->setProperty("blockId", blockId);
    modeDetail->setProperty("bypassMode", stellarr::bypassModeToString(block->getBypassMode()));
    emitToJs("blockBypassModeChanged", modeDetail);
}

void StellarrBridge::clearAllDirtyStates()
{
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                pluginBlock->saveCurrentState();
                emitBlockStates(blockId, pluginBlock);
            }
        }
    }
}
