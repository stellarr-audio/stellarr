#include "ParamHandler.h"
#include "../StellarrProcessor.h"
#include "../blocks/Block.h"
#include "../blocks/PluginBlock.h"
#include "EventNames.h"
#include "internal/BlockLookup.h"

namespace stellarr::bridge {

using namespace stellarr::bridge::internal;

ParamHandler::ParamHandler(ParamHandlerContext c) : ctx(c) {}

// -- Block parameter handlers -------------------------------------------------
// These all follow the same pattern: parse blockId + value, find block, set
// param, mark dirty if plugin block, emit change event.

void ParamHandler::handleSetBlockParam(const juce::var& json,
                                       const juce::String& paramName,
                                       std::function<void(stellarr::Block*, const juce::var&)> setter,
                                       const juce::String& eventName,
                                       std::function<juce::var(stellarr::Block*)> getter)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (block == nullptr) return;

    setter(block, obj->getProperty(paramName));
    markDirtyAndEmit(blockId);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty(paramName, getter(block));
    ctx.emit.emit(eventName, detail);
}

void ParamHandler::handleSetBlockMix(const juce::var& json)
{
    handleSetBlockParam(json, "mix",
        [](stellarr::Block* b, const juce::var& v) { b->setMix(static_cast<float>(v)); },
        events::BlockMixChanged,
        [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getMix())); });
}

void ParamHandler::handleSetBlockBalance(const juce::var& json)
{
    handleSetBlockParam(json, "balance",
        [](stellarr::Block* b, const juce::var& v) { b->setBalance(static_cast<float>(v)); },
        events::BlockBalanceChanged,
        [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getBalance())); });
}

void ParamHandler::handleSetBlockLevel(const juce::var& json)
{
    handleSetBlockParam(json, "level",
        [](stellarr::Block* b, const juce::var& v) { b->setLevelDb(static_cast<float>(v)); },
        events::BlockLevelChanged,
        [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getLevelDb())); });
}

void ParamHandler::handleSetBlockBypassMode(const juce::var& json)
{
    handleSetBlockParam(json, "bypassMode",
        [](stellarr::Block* b, const juce::var& v) {
            b->setBypassMode(stellarr::bypassModeFromString(v.toString()));
        },
        events::BlockBypassModeChanged,
        [](stellarr::Block* b) {
            return juce::var(stellarr::bypassModeToString(b->getBypassMode()));
        });
}

// -- Block state handlers (save/add/recall/delete) ----------------------------

void ParamHandler::handleBlockStateEvent(const juce::var& json, const juce::String& action)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* pluginBlock = findPluginBlock(ctx.blockNodeMap, ctx.processor, blockId);
    if (pluginBlock == nullptr) return;

    bool activeIndexChanged = false;

    if (action == "save")
    {
        pluginBlock->saveCurrentState();
    }
    else if (action == "add")
    {
        if (pluginBlock->addState())
            activeIndexChanged = true;
    }
    else if (action == "recall")
    {
        auto index = static_cast<int>(obj->getProperty("index"));
        if (pluginBlock->recallState(index))
        {
            emitBlockParams(blockId, pluginBlock);
            activeIndexChanged = true;
        }
    }
    else if (action == "delete")
    {
        auto index = static_cast<int>(obj->getProperty("index"));
        if (pluginBlock->deleteState(index))
        {
            emitBlockParams(blockId, pluginBlock);
            activeIndexChanged = true;

            // Deleting state at `index` shifts all higher indices down by one.
            // PluginBlock::deleteState() shifts its own activeStateIndex; mirror
            // the same on every scene's stored map so inactive scenes do not
            // keep stale references to removed/shifted state slots, and on any
            // MIDI blockState mappings. Both kinds of bookkeeping live on
            // StellarrBridge, so route via the context callback.
            if (ctx.onBlockStateDeleted)
                ctx.onBlockStateDeleted(blockId, index);
        }
    }

    emitBlockStates(blockId, pluginBlock);

    // The active scene's blockStateMap drives the rewire-dot prediction in the
    // UI. When a manual block state change shifts the active State index, sync
    // it into the active scene and re-emit so the dropdown's dots reflect
    // current rewire behaviour.
    if (activeIndexChanged && ctx.onActiveStateChanged)
        ctx.onActiveStateChanged(blockId, pluginBlock->getActiveStateIndex());
}

// -- Emit helpers -------------------------------------------------------------

void ParamHandler::emitBlockStates(const juce::String& blockId, stellarr::PluginBlock* pluginBlock)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("numStates", pluginBlock->getNumStates());
    detail->setProperty("activeStateIndex", pluginBlock->getActiveStateIndex());

    juce::Array<juce::var> dirtyArr;
    for (int d : pluginBlock->getDirtyStates())
        dirtyArr.add(d);
    detail->setProperty("dirtyStates", dirtyArr);

    ctx.emit.emit(events::BlockStateChanged, detail);
}

void ParamHandler::emitBlockParams(const juce::String& blockId, stellarr::Block* block)
{
    auto* mixDetail = new juce::DynamicObject();
    mixDetail->setProperty("blockId", blockId);
    mixDetail->setProperty("mix", static_cast<double>(block->getMix()));
    ctx.emit.emit(events::BlockMixChanged, mixDetail);

    auto* balDetail = new juce::DynamicObject();
    balDetail->setProperty("blockId", blockId);
    balDetail->setProperty("balance", static_cast<double>(block->getBalance()));
    ctx.emit.emit(events::BlockBalanceChanged, balDetail);

    auto* lvlDetail = new juce::DynamicObject();
    lvlDetail->setProperty("blockId", blockId);
    lvlDetail->setProperty("level", static_cast<double>(block->getLevelDb()));
    ctx.emit.emit(events::BlockLevelChanged, lvlDetail);

    auto* bypDetail = new juce::DynamicObject();
    bypDetail->setProperty("blockId", blockId);
    bypDetail->setProperty("bypassed", block->isBypassed());
    ctx.emit.emit(events::BlockBypassChanged, bypDetail);

    auto* modeDetail = new juce::DynamicObject();
    modeDetail->setProperty("blockId", blockId);
    modeDetail->setProperty("bypassMode", stellarr::bypassModeToString(block->getBypassMode()));
    ctx.emit.emit(events::BlockBypassModeChanged, modeDetail);
}

void ParamHandler::clearAllDirtyStates()
{
    for (auto& [blockId, nodeId] : ctx.blockNodeMap)
    {
        if (auto* node = ctx.processor.getGraph().getNodeForId(nodeId))
        {
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                pluginBlock->saveCurrentState();
                emitBlockStates(blockId, pluginBlock);
            }
        }
    }
}

void ParamHandler::markDirtyAndEmit(const juce::String& blockId)
{
    auto nodeIt = ctx.blockNodeMap.find(blockId);
    if (nodeIt == ctx.blockNodeMap.end()) return;

    if (auto* node = ctx.processor.getGraph().getNodeForId(nodeIt->second))
    {
        if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
        {
            pb->markDirty();
            emitBlockStates(blockId, pb);
        }
    }
}

} // namespace stellarr::bridge
