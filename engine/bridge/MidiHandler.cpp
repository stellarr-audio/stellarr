#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../blocks/Block.h"
#include "../blocks/PluginBlock.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include <cmath>

void StellarrBridge::setupMidiMapper()
{
    if (processor == nullptr) return;

    auto& mapper = processor->getMidiMapper();

    // All callbacks below run on the message thread (drained from
    // mapper.drainOutboundEvents() called by the editor's timer). The audio
    // thread enqueues OutboundEvents into a lock-free SPSC fifo and never
    // touches std::function or MessageManager.
    //
    // Each callback that mutates per-preset state (graph, scenes, block
    // params, tuner) early-returns when an async restore is in flight —
    // Phase 2 will clearGraph() and reload everything from the new preset,
    // so applying the MIDI mutation now would silently lose it. Mirrors the
    // pendingRestore gate at the top of handleEvent for WebView events.

    mapper.onPresetChange = [this](int index) {
        // handleLoadPresetByIndex is itself protected by restoreSession's
        // try_lock — a competing call during an in-flight restore is dropped
        // there. Still, dedupe and bookkeeping are skipped on rejection, so
        // the call is harmless either way.
        auto json = juce::JSON::parse("{\"index\":" + juce::String(index) + "}");
        handleLoadPresetByIndex(json);
    };

    mapper.onSceneSwitch = [this](int index) {
        if (pendingRestore.has_value()) return;
        auto json = juce::JSON::parse("{\"index\":" + juce::String(index) + "}");
        handleRecallScene(json);
    };

    mapper.onBlockBypass = [this](const juce::String& blockId, bool state) {
        if (pendingRestore.has_value()) return;
        auto* block = findBlock(blockId);
        if (block == nullptr) return;

        block->setBypassed(state);
        markDirtyAndEmit(blockId, block);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("bypassed", state);
        emitToJs("blockBypassChanged", detail);
    };

    mapper.onBlockMix = [this](const juce::String& blockId, float value) {
        if (pendingRestore.has_value()) return;
        auto* block = findBlock(blockId);
        if (block == nullptr) return;

        block->setMix(value);
        markDirtyAndEmit(blockId, block);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("mix", static_cast<double>(value));
        emitToJs("blockMixChanged", detail);
    };

    mapper.onBlockBalance = [this](const juce::String& blockId, float value) {
        if (pendingRestore.has_value()) return;
        auto* block = findBlock(blockId);
        if (block == nullptr) return;

        block->setBalance(value);
        markDirtyAndEmit(blockId, block);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("balance", static_cast<double>(value));
        emitToJs("blockBalanceChanged", detail);
    };

    mapper.onBlockLevel = [this](const juce::String& blockId, float levelDb) {
        if (pendingRestore.has_value()) return;
        auto* block = findBlock(blockId);
        if (block == nullptr) return;

        block->setLevelDb(levelDb);
        markDirtyAndEmit(blockId, block);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("level", static_cast<double>(levelDb));
        emitToJs("blockLevelChanged", detail);
    };

    mapper.onTunerToggle = [this](bool enabled) {
        if (pendingRestore.has_value()) return;
        tunerActive = enabled;
        for (auto& [blockId, nodeId] : blockNodeMap)
        {
            if (auto* node = processor->getGraph().getNodeForId(nodeId))
            {
                if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
                    inputBlock->setTunerEnabled(enabled);
                if (auto* outputBlock = dynamic_cast<stellarr::OutputBlock*>(node->getProcessor()))
                    outputBlock->setTunerMute(enabled);
            }
        }
    };

    mapper.onBlockState = [this](const juce::String& blockId, int stateIndex) {
        if (pendingRestore.has_value()) return;
        auto* pluginBlock = findPluginBlock(blockId);
        if (pluginBlock == nullptr) return;

        // Skip when the requested state is already active. Controllers that
        // re-send 127 or stream values above the threshold would otherwise
        // re-apply the plugin's stored state and re-emit on every message —
        // a CPU spike and potential audio hiccup during live use.
        if (stateIndex == pluginBlock->getActiveStateIndex()) return;

        if (pluginBlock->recallState(stateIndex))
        {
            emitBlockParams(blockId, pluginBlock);
            emitBlockStates(blockId, pluginBlock);

            // Mirror handleBlockStateEvent("recall"): sync the active scene's
            // blockStateMap so the rewire-dot prediction in the scene dropdown
            // reflects the new state. Without this, MIDI-driven state changes
            // diverge silently from the visible scene indicator.
            if (activeSceneIndex >= 0
                && activeSceneIndex < static_cast<int>(scenes.size()))
            {
                scenes[static_cast<size_t>(activeSceneIndex)].blockStateMap[blockId]
                    = pluginBlock->getActiveStateIndex();
                emitScenes();
            }
        }
    };

    mapper.onLearnComplete = [this](int channel, int cc) {
        auto* detail = new juce::DynamicObject();
        detail->setProperty("channel", channel);
        detail->setProperty("cc", cc);
        emitToJs("midiLearnComplete", detail);
        emitMidiMappings();
    };
}

void StellarrBridge::emitMidiMappings()
{
    if (processor == nullptr) return;

    auto& mapper = processor->getMidiMapper();
    auto* detail = new juce::DynamicObject();

    juce::Array<juce::var> arr;
    for (int i = 0; i < mapper.getNumMappings(); ++i)
    {
        auto& m = mapper.getMapping(i);
        auto* obj = new juce::DynamicObject();
        obj->setProperty("channel", m.channel);
        obj->setProperty("cc", m.ccNumber);
        obj->setProperty("target", MidiMapper::targetToString(m.target));
        if (m.blockId.isNotEmpty())
            obj->setProperty("blockId", m.blockId);
        if (m.targetIndex >= 0)
            obj->setProperty("targetIndex", m.targetIndex);
        if (m.ccMin != 0)
            obj->setProperty("ccMin", m.ccMin);
        if (m.ccMax != 127)
            obj->setProperty("ccMax", m.ccMax);
        if (! std::isnan(m.paramMin))
            obj->setProperty("paramMin", static_cast<double>(m.paramMin));
        if (! std::isnan(m.paramMax))
            obj->setProperty("paramMax", static_cast<double>(m.paramMax));
        if (m.curve != MidiMapper::Curve::Linear)
            obj->setProperty("curve", MidiMapper::curveToString(m.curve));
        if (m.threshold != 64)
            obj->setProperty("threshold", m.threshold);
        arr.add(juce::var(obj));
    }

    detail->setProperty("mappings", arr);
    detail->setProperty("learning", mapper.isLearning());
    emitToJs("midiMappingsChanged", detail);
}
