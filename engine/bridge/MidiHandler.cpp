#include "MidiHandler.h"
#include "../StellarrProcessor.h"
#include "../blocks/Block.h"
#include "../blocks/PluginBlock.h"
#include "BridgeJson.h"
#include "EventNames.h"
#include "internal/BlockLookup.h"
#include <cmath>
#include <limits>

namespace stellarr::bridge {

using namespace stellarr::bridge::internal;

MidiHandler::MidiHandler(MidiHandlerContext c) : ctx(c)
{
    registerMapperCallbacks();
}

MidiHandler::~MidiHandler()
{
    clearMapperCallbacks();
}

void MidiHandler::registerMapperCallbacks()
{
    auto& mapper = ctx.processor.getMidiMapper();

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
        // loadPresetByIndex is itself protected by restoreSession's try_lock —
        // a competing call during an in-flight restore is dropped there.
        // Still, dedupe and bookkeeping are skipped on rejection, so the call
        // is harmless either way.
        ctx.loadPresetByIndex(index);
    };

    mapper.onSceneSwitch = [this](int index) {
        if (ctx.isRestoring()) return;
        ctx.recallSceneByIndex(index);
    };

    mapper.onBlockBypass = [this](const juce::String& blockId, bool state) {
        if (ctx.isRestoring()) return;
        auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
        if (block == nullptr) return;

        block->setBypassed(state);
        ctx.markBlockDirty(blockId);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("bypassed", state);
        ctx.emit.emit(events::BlockBypassChanged, detail);
    };

    mapper.onBlockMix = [this](const juce::String& blockId, float value) {
        if (ctx.isRestoring()) return;
        auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
        if (block == nullptr) return;

        block->setMix(value);
        ctx.markBlockDirty(blockId);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("mix", static_cast<double>(value));
        ctx.emit.emit(events::BlockMixChanged, detail);
    };

    mapper.onBlockBalance = [this](const juce::String& blockId, float value) {
        if (ctx.isRestoring()) return;
        auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
        if (block == nullptr) return;

        block->setBalance(value);
        ctx.markBlockDirty(blockId);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("balance", static_cast<double>(value));
        ctx.emit.emit(events::BlockBalanceChanged, detail);
    };

    mapper.onBlockLevel = [this](const juce::String& blockId, float levelDb) {
        if (ctx.isRestoring()) return;
        auto* block = findBlock(ctx.blockNodeMap, ctx.processor, blockId);
        if (block == nullptr) return;

        block->setLevelDb(levelDb);
        ctx.markBlockDirty(blockId);

        auto* detail = new juce::DynamicObject();
        detail->setProperty("blockId", blockId);
        detail->setProperty("level", static_cast<double>(levelDb));
        ctx.emit.emit(events::BlockLevelChanged, detail);
    };

    mapper.onTunerToggle = [this](bool enabled) {
        if (ctx.isRestoring()) return;
        ctx.setTunerActiveOnAllBlocks(enabled);

        // Notify the UI so it can switch to / from the Tuner tab and keep
        // its tab state in sync. The user pressing a foot-switch CC expects
        // to see the tuner appear, not just have engine-side pitch
        // detection running silently.
        auto* detail = new juce::DynamicObject();
        detail->setProperty("active", enabled);
        ctx.emit.emit(events::TunerActiveState, detail);
    };

    mapper.onBlockState = [this](const juce::String& blockId, int stateIndex) {
        if (ctx.isRestoring()) return;
        auto* pluginBlock = findPluginBlock(ctx.blockNodeMap, ctx.processor, blockId);
        if (pluginBlock == nullptr) return;

        // Skip when the requested state is already active. Controllers that
        // re-send 127 or stream values above the threshold would otherwise
        // re-apply the plugin's stored state and re-emit on every message —
        // a CPU spike and potential audio hiccup during live use.
        if (stateIndex == pluginBlock->getActiveStateIndex()) return;

        if (pluginBlock->recallState(stateIndex))
        {
            ctx.emitBlockParams(blockId, pluginBlock);
            ctx.emitBlockStates(blockId, pluginBlock);

            // Mirror handleBlockStateEvent("recall"): sync the active scene's
            // blockStateMap so the rewire-dot prediction in the scene dropdown
            // reflects the new state. Without this, MIDI-driven state changes
            // diverge silently from the visible scene indicator.
            ctx.mirrorActiveStateInScene(blockId, pluginBlock->getActiveStateIndex());
        }
    };

    mapper.onLearnComplete = [this](int channel, int cc) {
        auto* detail = new juce::DynamicObject();
        detail->setProperty("channel", channel);
        detail->setProperty("cc", cc);
        ctx.emit.emit(events::MidiLearnComplete, detail);
        emitMidiMappings();
    };
}

void MidiHandler::clearMapperCallbacks()
{
    // Sever the std::function captures before this handler dies. MidiMapper
    // holds copies of these lambdas (which capture `this` of MidiHandler);
    // if a queued outbound event drained after destruction, calling the stale
    // copy would dereference a dangling MidiHandler. Resetting the mapper's
    // callback slots back to default-constructed std::function makes the
    // post-destruction drain a no-op.
    auto& mapper = ctx.processor.getMidiMapper();
    mapper.onPresetChange   = {};
    mapper.onSceneSwitch    = {};
    mapper.onBlockBypass    = {};
    mapper.onBlockMix       = {};
    mapper.onBlockBalance   = {};
    mapper.onBlockLevel     = {};
    mapper.onTunerToggle    = {};
    mapper.onBlockState     = {};
    mapper.onLearnComplete  = {};
}

void MidiHandler::emitMidiMappings()
{
    auto& mapper = ctx.processor.getMidiMapper();
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
    ctx.emit.emit(events::MidiMappingsChanged, detail);
}

// -- MIDI mapping handlers ----------------------------------------------------

void MidiHandler::handleAddMidiMapping(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    using namespace stellarr::bridge::json;
    MidiMapper::Mapping m;
    m.channel     = static_cast<int>(obj->getProperty("channel"));
    m.ccNumber    = static_cast<int>(obj->getProperty("cc"));
    m.target      = MidiMapper::targetFromString(obj->getProperty("target").toString());
    m.blockId     = obj->getProperty("blockId").toString();
    m.targetIndex = getOptInt        (*obj, "targetIndex", -1);
    m.ccMin       = getOptInt        (*obj, "ccMin",        0);
    m.ccMax       = getOptInt        (*obj, "ccMax",      127);
    m.paramMin    = getOptFloat      (*obj, "paramMin", std::numeric_limits<float>::quiet_NaN());
    m.paramMax    = getOptFloat      (*obj, "paramMax", std::numeric_limits<float>::quiet_NaN());
    m.curve       = MidiMapper::curveFromString(getOptString(*obj, "curve", "linear"));
    m.threshold   = getOptIntClamped (*obj, "threshold", 1, 127, 64);

    ctx.processor.getMidiMapper().addMapping(m);
    emitMidiMappings();
}

void MidiHandler::handleRemoveMidiMapping(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    ctx.processor.getMidiMapper().removeMapping(static_cast<int>(obj->getProperty("index")));
    emitMidiMappings();
}

void MidiHandler::handleClearMidiMappings()
{
    ctx.processor.getMidiMapper().clearAll();
    emitMidiMappings();
}

void MidiHandler::handleStartMidiLearn(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    using namespace stellarr::bridge::json;
    MidiMapper::LearnArgs args;
    args.target      = MidiMapper::targetFromString(obj->getProperty("target").toString());
    args.blockId     = obj->getProperty("blockId").toString();
    args.targetIndex = getOptInt        (*obj, "targetIndex", -1);
    args.ccMin       = getOptInt        (*obj, "ccMin",        0);
    args.ccMax       = getOptInt        (*obj, "ccMax",      127);
    args.paramMin    = getOptFloat      (*obj, "paramMin", std::numeric_limits<float>::quiet_NaN());
    args.paramMax    = getOptFloat      (*obj, "paramMax", std::numeric_limits<float>::quiet_NaN());
    args.curve       = MidiMapper::curveFromString(getOptString(*obj, "curve", "linear"));
    args.threshold   = getOptIntClamped (*obj, "threshold", 1, 127, 64);

    ctx.processor.getMidiMapper().startLearn(args);
    emitMidiMappings();
}

void MidiHandler::handleCancelMidiLearn()
{
    ctx.processor.getMidiMapper().cancelLearn();
    emitMidiMappings();
}

void MidiHandler::handleSetMidiMonitorEnabled(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;
    ctx.processor.getMidiMapper().setMonitorEnabled(static_cast<bool>(obj->getProperty("enabled")));
}

void MidiHandler::handleInjectMidiCC(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;
    int ch  = static_cast<int>(obj->getProperty("channel")) + 1; // 0-indexed -> 1-indexed
    int cc  = static_cast<int>(obj->getProperty("cc"));
    int val = static_cast<int>(obj->getProperty("value"));
    ctx.processor.getMidiMapper().injectMidi(juce::MidiMessage::controllerEvent(ch, cc, val));
}

void MidiHandler::handleInjectMidiPC(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;
    int ch      = static_cast<int>(obj->getProperty("channel")) + 1; // 0-indexed -> 1-indexed
    int program = static_cast<int>(obj->getProperty("program"));
    ctx.processor.getMidiMapper().injectMidi(juce::MidiMessage::programChange(ch, program));
}

} // namespace stellarr::bridge
