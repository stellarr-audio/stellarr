#include "StellarrBridge.h"
#include "StellarrProcessor.h"
#include "StellarrPlatform.h"
#include "UpdaterShim.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/PluginBlock.h"
#include <cmath>
#include <limits>
#include <optional>

StellarrBridge::StellarrBridge() = default;
StellarrBridge::~StellarrBridge() = default;

void StellarrBridge::setProcessor(StellarrProcessor* proc)
{
    processor = proc;
    setupMidiMapper();
}

void StellarrBridge::setAppProperties(juce::ApplicationProperties* props)
{
    appProperties = props;
}

juce::WebBrowserComponent::Options StellarrBridge::configureOptions(juce::WebBrowserComponent::Options options)
{
    return options.withNativeFunction("sendToNative",
        [this](const juce::Array<juce::var>& args,
               juce::WebBrowserComponent::NativeFunctionCompletion completion)
        {
            if (args.size() >= 2)
                handleEvent(args[0].toString(), args[1]);
            else if (args.size() == 1)
                handleEvent(args[0].toString(), {});

            completion(juce::var("ok"));
        });
}

void StellarrBridge::setWebView(juce::WebBrowserComponent* browser)
{
    webView = browser;
}

// -- Event dispatch -----------------------------------------------------------

const StellarrBridge::EventTable& StellarrBridge::eventTable()
{
    static const EventTable t = []
    {
        EventTable m;
        // Lifecycle ----------------------------------------------------
        m["bridgeReady"]              = { [](StellarrBridge& b, const juce::var&)   { b.handleBridgeReady(); }, false };
        m["uiReady"]                  = { [](StellarrBridge& b, const juce::var&)   { if (b.onUiReady) b.onUiReady(); b.handleScreenshotSetup(); }, false };
        m["screenshotReady"]          = { [](StellarrBridge& b, const juce::var&)   { b.handleScreenshotReady(); }, false };
        // Software updates (Sparkle) ----------------------------------
        m["update/check"]             = { [](StellarrBridge& b, const juce::var&)   { b.handleUpdateCheck(); }, false };
        m["update/install"]           = { [](StellarrBridge& b, const juce::var&)   { b.handleUpdateInstall(); }, false };
        m["update/open-release-notes"]= { [](StellarrBridge& b, const juce::var& j) { b.handleUpdateOpenReleaseNotes(j); }, false };
        // Graph --------------------------------------------------------
        m["addBlock"]                 = { [](StellarrBridge& b, const juce::var& j) { b.handleAddBlock(j); }, true };
        m["removeBlock"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleRemoveBlock(j); }, true };
        m["moveBlock"]                = { [](StellarrBridge& b, const juce::var& j) { b.handleMoveBlock(j); }, true };
        m["addConnection"]            = { [](StellarrBridge& b, const juce::var& j) { b.handleAddConnection(j); }, true };
        m["removeConnection"]         = { [](StellarrBridge& b, const juce::var& j) { b.handleRemoveConnection(j); }, true };
        m["setBlockPlugin"]           = { [](StellarrBridge& b, const juce::var& j) { b.handleSetBlockPlugin(j); }, true };
        m["openPluginEditor"]         = { [](StellarrBridge& b, const juce::var& j) { b.handleOpenPluginEditor(j); }, false };
        m["copyBlock"]                = { [](StellarrBridge& b, const juce::var& j) { b.handleCopyBlock(j); }, true };
        m["pasteBlock"]               = { [](StellarrBridge& b, const juce::var& j) { b.handlePasteBlock(j); }, true };
        m["renameBlock"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleRenameBlock(j); }, true };
        m["setBlockColor"]            = { [](StellarrBridge& b, const juce::var& j) { b.handleSetBlockColor(j); }, true };
        // MIDI mappings ------------------------------------------------
        m["addMidiMapping"]           = { [](StellarrBridge& b, const juce::var& j) { b.handleAddMidiMapping(j); }, true };
        m["removeMidiMapping"]        = { [](StellarrBridge& b, const juce::var& j) { b.handleRemoveMidiMapping(j); }, true };
        m["clearMidiMappings"]        = { [](StellarrBridge& b, const juce::var&)   { b.handleClearMidiMappings(); }, true };
        m["getMidiMappings"]          = { [](StellarrBridge& b, const juce::var&)   { b.emitMidiMappings(); }, false };
        m["startMidiLearn"]           = { [](StellarrBridge& b, const juce::var& j) { b.handleStartMidiLearn(j); }, true };
        m["cancelMidiLearn"]          = { [](StellarrBridge& b, const juce::var&)   { b.handleCancelMidiLearn(); }, true };
        m["setMidiMonitorEnabled"]    = { [](StellarrBridge& b, const juce::var& j) { b.handleSetMidiMonitorEnabled(j); }, false };
        m["injectMidiCC"]             = { [](StellarrBridge& b, const juce::var& j) { b.handleInjectMidiCC(j); }, false };
        // Plugin management -------------------------------------------
        m["scanPlugins"]              = { [](StellarrBridge& b, const juce::var&)   { b.handleScanPlugins(); }, true };
        m["getScanDirectories"]       = { [](StellarrBridge& b, const juce::var&)   { b.handleGetScanDirectories(); }, false };
        m["pickScanDirectory"]        = { [](StellarrBridge& b, const juce::var&)   { b.handlePickScanDirectory(); }, true };
        m["removeScanDirectory"]      = { [](StellarrBridge& b, const juce::var& j) { b.handleRemoveScanDirectory(j); }, true };
        // Telemetry ----------------------------------------------------
        m["getTelemetryEnabled"]      = { [](StellarrBridge& b, const juce::var&)   { b.handleGetTelemetryEnabled(); }, false };
        m["setTelemetryEnabled"]      = { [](StellarrBridge& b, const juce::var& j) { b.handleSetTelemetryEnabled(j); }, false };
        // Tuner settings ----------------------------------------------
        m["getReferencePitch"]        = { [](StellarrBridge& b, const juce::var&)   { b.handleGetReferencePitch(); }, false };
        m["setReferencePitch"]        = { [](StellarrBridge& b, const juce::var& j) { b.handleSetReferencePitch(j); }, false };
        // Presets ------------------------------------------------------
        m["newSession"]               = { [](StellarrBridge& b, const juce::var&)   { b.handleNewSession(); }, true };
        m["saveSession"]              = { [](StellarrBridge& b, const juce::var&)   { b.handleSaveSession(); }, true };
        m["saveSessionQuiet"]         = { [](StellarrBridge& b, const juce::var&)   { b.handleSaveSessionQuiet(); }, true };
        m["loadSession"]              = { [](StellarrBridge& b, const juce::var&)   { b.handleLoadSession(); }, false };
        m["pickPresetDirectory"]      = { [](StellarrBridge& b, const juce::var&)   { b.handlePickPresetDirectory(); }, true };
        m["loadPresetByIndex"]        = { [](StellarrBridge& b, const juce::var& j) { b.handleLoadPresetByIndex(j); }, false };
        m["renamePreset"]             = { [](StellarrBridge& b, const juce::var& j) { b.handleRenamePreset(j); }, true };
        m["deletePreset"]             = { [](StellarrBridge& b, const juce::var& j) { b.handleDeletePreset(j); }, true };
        m["getPresetList"]            = { [](StellarrBridge& b, const juce::var&)   { b.handleGetPresetList(); }, false };
        m["setGridSize"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleSetGridSize(j); }, true };
        // Scenes -------------------------------------------------------
        m["addScene"]                 = { [](StellarrBridge& b, const juce::var&)   { b.handleAddScene(); }, true };
        m["recallScene"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleRecallScene(j); }, true };
        m["saveScene"]                = { [](StellarrBridge& b, const juce::var& j) { b.handleSaveScene(j); }, true };
        m["renameScene"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleRenameScene(j); }, true };
        m["deleteScene"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleDeleteScene(j); }, true };
        // Input block controls ----------------------------------------
        m["toggleTestTone"]           = { [](StellarrBridge& b, const juce::var& j) { b.handleToggleTestTone(j); }, true };
        m["getTestToneSamples"]       = { [](StellarrBridge& b, const juce::var&)   { b.handleGetTestToneSamples(); }, false };
        m["setTestToneSample"]        = { [](StellarrBridge& b, const juce::var& j) { b.handleSetTestToneSample(j); }, true };
        m["setTunerEnabled"]          = { [](StellarrBridge& b, const juce::var& j) { b.handleSetTunerEnabled(j); }, true };
        // Block parameters --------------------------------------------
        m["setBlockMix"]              = { [](StellarrBridge& b, const juce::var& j) { b.handleSetBlockMix(j); }, true };
        m["setBlockBalance"]          = { [](StellarrBridge& b, const juce::var& j) { b.handleSetBlockBalance(j); }, true };
        m["setBlockLevel"]            = { [](StellarrBridge& b, const juce::var& j) { b.handleSetBlockLevel(j); }, true };
        m["toggleBlockBypass"]        = { [](StellarrBridge& b, const juce::var& j) { b.handleToggleBlockBypass(j); }, true };
        m["setBlockBypassMode"]       = { [](StellarrBridge& b, const juce::var& j) { b.handleSetBlockBypassMode(j); }, true };
        // Block states -------------------------------------------------
        m["saveBlockState"]           = { [](StellarrBridge& b, const juce::var& j) { b.handleBlockStateEvent(j, "save"); }, true };
        m["addBlockState"]            = { [](StellarrBridge& b, const juce::var& j) { b.handleBlockStateEvent(j, "add"); }, true };
        m["recallBlockState"]         = { [](StellarrBridge& b, const juce::var& j) { b.handleBlockStateEvent(j, "recall"); }, true };
        m["deleteBlockState"]         = { [](StellarrBridge& b, const juce::var& j) { b.handleBlockStateEvent(j, "delete"); }, true };
        // Loudness metering -------------------------------------------
        m["setSelectedBlock"]         = { [](StellarrBridge& b, const juce::var& j) { b.handleSetSelectedBlock(j); }, false };
        m["setTargetLufs"]            = { [](StellarrBridge& b, const juce::var& j) { b.handleSetTargetLufs(j); }, true };
        m["setLufsWindow"]            = { [](StellarrBridge& b, const juce::var& j) { b.handleSetLufsWindow(j); }, false };
        return m;
    }();
    return t;
}

void StellarrBridge::handleEvent(const juce::String& eventName, const juce::var& payload)
{
    auto json = payload.isString()
        ? juce::JSON::parse(payload.toString())
        : payload;

    auto it = eventTable().find(eventName);
    if (it == eventTable().end())
    {
        DBG("StellarrBridge: unknown event '" << eventName << "'");
        return;
    }

    // While an async restoreSession is in flight (Phase 1 yielding between
    // plugin loads), drop any event that would mutate the graph, scenes, MIDI
    // mappings, or persisted preset state. Phase 2 calls clearGraph() and
    // overwrites everything; letting those edits run during the load window
    // would silently lose them on the next async tick. Read-only events and
    // the lock-guarded preset-load events (loadPresetByIndex, loadSession —
    // both reject themselves via restoreSession's try_lock) pass through.
    if (pendingRestore.has_value() && it->second.dropDuringRestore)
    {
        DBG("StellarrBridge: dropping '" << eventName << "' while preset restore is in flight");
        return;
    }

    it->second.handler(*this, json);
}

// -- Lifted inline handlers (Phase 3) -----------------------------------------
// Bodies lifted character-for-character from the old if/else if chain. The
// `processor != nullptr` guards that used to live in the if-clause now sit at
// the top of each function. Phase 4 will move these into engine/bridge/*.cpp.

void StellarrBridge::handleRenameBlock(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto name = obj->getProperty("name").toString();
    auto* block = findBlock(blockId);
    if (block == nullptr) return;

    block->setDisplayName(name);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("displayName", name);
    emitToJs("blockRenamed", detail);
}

void StellarrBridge::handleSetBlockColor(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto color = obj->getProperty("color").toString();
    auto* block = findBlock(blockId);
    if (block == nullptr) return;

    block->setBlockColor(color);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("blockColor", color);
    emitToJs("blockColorChanged", detail);
}

void StellarrBridge::handleToggleTestTone(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
    {
        if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
        {
            bool enabled = !inputBlock->isTestToneEnabled();
            inputBlock->setTestToneEnabled(enabled);

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("enabled", enabled);
            emitToJs("testToneChanged", detail);
        }
    }
}

void StellarrBridge::handleGetTestToneSamples()
{
    auto samplesDir = stellarrGetBundleResource("samples");
    juce::Array<juce::var> files;

    if (samplesDir.isDirectory())
    {
        for (auto& f : samplesDir.findChildFiles(juce::File::findFiles, false, "*.wav"))
        {
            auto name = f.getFileNameWithoutExtension();
            if (name.equalsIgnoreCase("placeholder")) continue; // skip submodule placeholder file
            files.add(juce::var(name));
        }
    }

    // Add "Synth (Default)" as first option
    juce::Array<juce::var> sorted;
    sorted.add(juce::var("Synth (Default)"));
    for (auto& f : files)
        sorted.add(f);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("samples", sorted);
    emitToJs("testToneSamplesUpdated", detail);
}

void StellarrBridge::handleSetTestToneSample(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto sampleName = obj->getProperty("sample").toString();
    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
    {
        if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
        {
            if (sampleName == "Synth (Default)" || sampleName.isEmpty())
            {
                inputBlock->clearTestToneSample();
            }
            else
            {
                auto samplesDir = stellarrGetBundleResource("samples");
                auto file = samplesDir.getChildFile(sampleName + ".wav");
                inputBlock->loadTestToneSample(file);
            }

            auto* detail = new juce::DynamicObject();
            detail->setProperty("blockId", blockId);
            detail->setProperty("sample", inputBlock->isUsingSample()
                ? inputBlock->getCurrentSampleName() : juce::String("Synth (Default)"));
            emitToJs("testToneSampleChanged", detail);
        }
    }
}

void StellarrBridge::handleSetTunerEnabled(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    bool enabled = static_cast<bool>(obj->getProperty("enabled"));
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
}

void StellarrBridge::handleSetBlockMix(const juce::var& json)
{
    handleSetBlockParam(json, "mix",
        [](stellarr::Block* b, const juce::var& v) { b->setMix(static_cast<float>(v)); },
        "blockMixChanged",
        [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getMix())); });
}

void StellarrBridge::handleSetBlockBalance(const juce::var& json)
{
    handleSetBlockParam(json, "balance",
        [](stellarr::Block* b, const juce::var& v) { b->setBalance(static_cast<float>(v)); },
        "blockBalanceChanged",
        [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getBalance())); });
}

void StellarrBridge::handleSetBlockLevel(const juce::var& json)
{
    handleSetBlockParam(json, "level",
        [](stellarr::Block* b, const juce::var& v) { b->setLevelDb(static_cast<float>(v)); },
        "blockLevelChanged",
        [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getLevelDb())); });
}

void StellarrBridge::handleToggleBlockBypass(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto* block = findBlock(blockId);
    if (block == nullptr) return;

    bool newState = !block->isBypassed();
    block->setBypassed(newState);
    markDirtyAndEmit(blockId, block);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blockId", blockId);
    detail->setProperty("bypassed", newState);
    emitToJs("blockBypassChanged", detail);
}

void StellarrBridge::handleSetBlockBypassMode(const juce::var& json)
{
    handleSetBlockParam(json, "bypassMode",
        [](stellarr::Block* b, const juce::var& v) {
            b->setBypassMode(stellarr::bypassModeFromString(v.toString()));
        },
        "blockBypassModeChanged",
        [](stellarr::Block* b) {
            return juce::var(stellarr::bypassModeToString(b->getBypassMode()));
        });
}

// -- Helpers ------------------------------------------------------------------

void StellarrBridge::connectIOBlock(const juce::String& type,
                                    juce::AudioProcessorGraph::NodeID nodeId,
                                    juce::AudioProcessorGraph::UpdateKind update)
{
    // Non-IO types (plugin/vst) share this entry point via paste / restore
    // paths but don't wire to the IO graph nodes — bail before touching the
    // default bypass so a plugin-only restore or paste doesn't silently
    // sever audio on a graph that still relies on the ctor's
    // audioInput → audioOutput passthrough.
    if (type != "input" && type != "output") return;

    // Tear down the default audioInput → audioOutput bypass before wiring an
    // IO block. StellarrProcessor's ctor adds that direct connection so a fresh
    // app boot still passes audio; once a real input/output block is added,
    // it must go — otherwise the dry signal leaks past the block chain (and
    // outside any Block::level / Block::bypass logic). disconnectBlocks is a
    // no-op if the connection is already gone, so it's safe to call from
    // every IO-wiring path.
    processor->disconnectBlocks(processor->getAudioInputNodeId(),
                                processor->getAudioOutputNodeId(), update);

    if (type == "input")
    {
        processor->connectBlocks(processor->getAudioInputNodeId(), nodeId, 2, update);
        processor->getGraph().addConnection({
            {processor->getMidiInputNodeId(), juce::AudioProcessorGraph::midiChannelIndex},
            {nodeId, juce::AudioProcessorGraph::midiChannelIndex}
        }, update);
    }
    else if (type == "output")
    {
        processor->connectBlocks(nodeId, processor->getAudioOutputNodeId(), 2, update);
        processor->getGraph().addConnection({
            {nodeId, juce::AudioProcessorGraph::midiChannelIndex},
            {processor->getMidiOutputNodeId(), juce::AudioProcessorGraph::midiChannelIndex}
        }, update);
    }
}

void StellarrBridge::restoreBlockPlugin(juce::AudioProcessorGraph::NodeID nodeId,
                                        const juce::String& pluginId,
                                        const juce::String& savedPluginName)
{
    if (pluginId.isEmpty()) return;

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
        else
        {
            pluginBlock->setPluginMissing(true);
            pluginBlock->setMissingPluginName(
                savedPluginName.isNotEmpty() ? savedPluginName : pluginId);
        }
    }
}

void StellarrBridge::emitToJs(const juce::String& eventName, juce::DynamicObject* detail)
{
    // Wrap the raw DynamicObject* once so the ReferenceCountedObjectPtr inside
    // `data` keeps it alive across both the interceptor call and the async
    // WebView emit. Building two separate juce::var's around the same raw
    // pointer would let the first temporary's destructor delete the object
    // before the second wrapper takes ownership.
    auto data = juce::var(detail);

    if (emitInterceptor) emitInterceptor(eventName, data);

    if (webView == nullptr) return;

    auto eventId = juce::Identifier(eventName);

    juce::MessageManager::callAsync([this, eventId, data]()
    {
        if (webView != nullptr)
            webView->emitEventIfBrowserIsVisible(eventId, data);
    });
}

void StellarrBridge::emitToJsSync(const juce::String& eventName, juce::DynamicObject* detail)
{
    // Same lifetime guard as emitToJs.
    auto data = juce::var(detail);

    if (emitInterceptor) emitInterceptor(eventName, data);

    if (webView == nullptr) return;

    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    auto eventId = juce::Identifier(eventName);
    webView->emitEventIfBrowserIsVisible(eventId, data);
}

stellarr::Block* StellarrBridge::findBlock(const juce::String& blockId)
{
    if (processor == nullptr) return nullptr;

    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return nullptr;

    auto* node = processor->getGraph().getNodeForId(nodeIt->second);
    if (node == nullptr) return nullptr;

    return dynamic_cast<stellarr::Block*>(node->getProcessor());
}

stellarr::PluginBlock* StellarrBridge::findPluginBlock(const juce::String& blockId)
{
    if (processor == nullptr) return nullptr;

    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return nullptr;

    auto* node = processor->getGraph().getNodeForId(nodeIt->second);
    if (node == nullptr) return nullptr;

    return dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
}

void StellarrBridge::markDirtyAndEmit(const juce::String& blockId, stellarr::Block* /*block*/)
{
    if (processor == nullptr) return;

    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
    {
        if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
        {
            pb->markDirty();
            emitBlockStates(blockId, pb);
        }
    }
}

// -- Startup and state broadcast ----------------------------------------------

void StellarrBridge::sendStartupProgress(const juce::String& status, int progress)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("status", status);
    detail->setProperty("progress", progress);
    emitToJs("startupProgress", detail);
}

void StellarrBridge::handleBridgeReady()
{
    sendStartupProgress("Connecting to engine...", 10);
    sendWelcome();

    auto* cfg = new juce::DynamicObject();
   #if STELLARR_IS_DEV
    cfg->setProperty("flavour", "dev");
   #else
    cfg->setProperty("flavour", "prod");
   #endif
    emitToJs("appConfig", cfg);

    handleGetTelemetryEnabled();
    handleGetReferencePitch();

    // Restore LUFS window from settings
    if (appProperties != nullptr)
    {
        auto savedWindow = appProperties->getUserSettings()->getValue("lufsWindow", "shortTerm");
        if (savedWindow != "momentary") savedWindow = "shortTerm";
        lufsWindow = savedWindow;

        auto* detail = new juce::DynamicObject();
        detail->setProperty("window", lufsWindow);
        emitToJs("lufsWindowState", detail);
    }

    juce::MessageManager::callAsync([this]()
    {
        sendStartupProgress("Scanning plugin libraries...", 30);
        sendScanDirectories();

        juce::MessageManager::callAsync([this]()
        {
            if (processor != nullptr)
            {
                processor->getPluginManager().scanPlugins();
                sendPluginList();
            }

            juce::MessageManager::callAsync([this]()
            {
                sendStartupProgress("Restoring session...", 60);

                // Final-step lambda — runs after the optional preset restore
                // either succeeds, fails, or is skipped. Seeds an empty
                // input/output graph if nothing was restored, then completes
                // startup.
                auto finishStartup = [this](bool restored)
                {
                    if (!restored && blockNodeMap.empty())
                    {
                        auto inputJson = juce::JSON::parse(R"({"type":"input","col":0,"row":2})");
                        auto outputJson = juce::JSON::parse(R"({"type":"output","col":11,"row":2})");
                        handleAddBlock(inputJson);
                        handleAddBlock(outputJson);
                    }

                    sendGraphState();
                    sendPresetList();

                    sendStartupProgress("Ready", 100);
                    emitToJs("startupComplete", new juce::DynamicObject());
                };

                if (appProperties != nullptr)
                {
                    auto* settings = appProperties->getUserSettings();

                    // Restore global MIDI mappings
                    auto globalMidi = settings->getValue("globalMidiMappings", "");
                    if (globalMidi.isNotEmpty() && processor != nullptr)
                        processor->getMidiMapper().loadGlobalMappings(juce::JSON::parse(globalMidi));

                    auto savedDir = settings->getValue("lastPresetDirectory", "");
                    auto savedIndex = settings->getIntValue("lastPresetIndex", -1);
                    if (savedDir.isNotEmpty())
                    {
                        presetDirectory = juce::File(savedDir);
                        handleGetPresetList();
                        currentPresetIndex = savedIndex;
                    }

                    auto savedFile = settings->getValue("lastPresetFile", "");
                    if (savedFile.isNotEmpty())
                    {
                        auto file = juce::File(savedFile);
                        if (file.existsAsFile())
                        {
                            auto jsonStr = file.loadFileAsString();
                            auto session = juce::JSON::parse(jsonStr);
                            if (session.getDynamicObject() != nullptr)
                            {
                                bool started = restoreSession(session,
                                    [this, file, finishStartup](bool ok)
                                    {
                                        if (ok)
                                        {
                                            lastPresetFile = file;
                                            presetDirectory = file.getParentDirectory();
                                            handleGetPresetList();

                                            for (int i = 0; i < presetFiles.size(); ++i)
                                            {
                                                if (presetFiles[i] == file.getFileName())
                                                {
                                                    currentPresetIndex = i;
                                                    break;
                                                }
                                            }
                                        }
                                        finishStartup(ok);
                                    });

                                if (started) return; // finishStartup runs in callback
                            }
                        }
                    }
                }

                // No restore was started — fall through to default seed.
                finishStartup(false);
            });
        });
    });
}

void StellarrBridge::sendWelcome()
{
    if (webView == nullptr) return;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("message", "Stellarr C++ engine is running");
    webView->emitEventIfBrowserIsVisible("welcome", juce::var(detail));
}

void StellarrBridge::sendGraphState()
{
    if (webView == nullptr) return;

    juce::Array<juce::var> blocksArray;
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        auto* blockObj = new juce::DynamicObject();
        blockObj->setProperty("id", blockId);
        blockObj->setProperty("nodeId", static_cast<int>(nodeId.uid));

        auto posIt = blockPositions.find(blockId);
        if (posIt != blockPositions.end())
        {
            blockObj->setProperty("col", posIt->second.first);
            blockObj->setProperty("row", posIt->second.second);
        }

        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                blockObj->setProperty("type", stellarr::blockTypeToString(block->getBlockType()));
                blockObj->setProperty("name", block->getName());
                if (block->getDisplayName().isNotEmpty())
                    blockObj->setProperty("displayName", block->getDisplayName());
                if (block->getBlockColor().isNotEmpty())
                    blockObj->setProperty("blockColor", block->getBlockColor());

                if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                {
                    blockObj->setProperty("pluginId", pluginBlock->getPluginIdentifier());
                    blockObj->setProperty("pluginName", pluginBlock->getPluginName());
                    blockObj->setProperty("pluginFormat", pluginBlock->getPluginFormat());
                    if (pluginBlock->isPluginMissing())
                        blockObj->setProperty("pluginMissing", true);
                    blockObj->setProperty("numStates", pluginBlock->getNumStates());
                    blockObj->setProperty("activeStateIndex", pluginBlock->getActiveStateIndex());
                    juce::Array<juce::var> dirtyArr;
                    for (int d : pluginBlock->getDirtyStates())
                        dirtyArr.add(d);
                    blockObj->setProperty("dirtyStates", dirtyArr);
                }

                blockObj->setProperty("mix", static_cast<double>(block->getMix()));
                blockObj->setProperty("balance", static_cast<double>(block->getBalance()));
                blockObj->setProperty("level", static_cast<double>(block->getLevelDb()));
                blockObj->setProperty("bypassed", block->isBypassed());
                blockObj->setProperty("bypassMode", stellarr::bypassModeToString(block->getBypassMode()));
            }
        }

        blocksArray.add(juce::var(blockObj));
    }

    juce::Array<juce::var> connectionsArray;
    std::map<juce::uint32, juce::String> nodeToBlock;
    for (auto& [blockId, nodeId] : blockNodeMap)
        nodeToBlock[nodeId.uid] = blockId;

    for (auto& conn : processor->getGraph().getConnections())
    {
        if (conn.source.channelIndex != 0) continue;

        auto srcIt = nodeToBlock.find(conn.source.nodeID.uid);
        auto dstIt = nodeToBlock.find(conn.destination.nodeID.uid);
        if (srcIt == nodeToBlock.end() || dstIt == nodeToBlock.end()) continue;

        auto* connObj = new juce::DynamicObject();
        connObj->setProperty("sourceId", srcIt->second);
        connObj->setProperty("destId", dstIt->second);
        connectionsArray.add(juce::var(connObj));
    }

    auto* state = new juce::DynamicObject();
    state->setProperty("blocks", blocksArray);
    state->setProperty("connections", connectionsArray);
    emitToJs("graphState", state);
    emitScenes();
}

// -- Screenshot automation ----------------------------------------------------

void StellarrBridge::handleScreenshotSetup()
{
    auto configFile = juce::File("/tmp/stellarr-screenshot-config.json");
    if (!configFile.existsAsFile()) return;

    // If a restore is already in flight (e.g. the startup last-session
    // restore is still loading plugins when uiReady fires), defer the entire
    // screenshot setup until that load completes — otherwise the capture
    // script's fixed timers can race the still-rebuilding graph. Leave the
    // config file in place so the deferred call can re-read it.
    if (pendingRestore.has_value())
    {
        runWhenRestoreIdle([this]() { handleScreenshotSetup(); });
        return;
    }

    auto screenshotConfig = juce::JSON::parse(configFile.loadFileAsString());
    configFile.deleteFile();

    auto* obj = screenshotConfig.getDynamicObject();
    if (obj == nullptr) return;

    // Load preset if specified. The screenshotSetup emit kicks off the UI's
    // capture timers, so it must NOT fire until any async preset restore has
    // finished — otherwise the screenshot script can race the still-loading
    // graph and capture a partial or stale state.
    auto presetPath = obj->getProperty("preset").toString();
    if (presetPath.isNotEmpty())
    {
        auto file = juce::File(presetPath);
        if (presetPath.startsWith("~"))
            file = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                       .getChildFile(presetPath.substring(2));

        if (file.existsAsFile())
        {
            auto jsonStr = file.loadFileAsString();
            auto session = juce::JSON::parse(jsonStr);
            if (session.getDynamicObject() != nullptr)
            {
                // Snapshot scene index up front — obj may not survive into
                // the async callback.
                std::optional<int> sceneIndex;
                if (obj->hasProperty("scene"))
                    sceneIndex = static_cast<int>(obj->getProperty("scene"));

                // Hold a ref-counted handle to the config so the JS-bound
                // payload survives across the async tick.
                juce::var configHandle = screenshotConfig;

                bool started = restoreSession(session,
                    [this, sceneIndex, configHandle](bool ok)
                {
                    if (ok)
                    {
                        sendGraphState();
                        if (sceneIndex.has_value())
                        {
                            auto sceneJson = juce::JSON::parse(
                                "{\"index\":" + juce::String(*sceneIndex) + "}");
                            handleRecallScene(sceneJson);
                        }
                    }
                    if (auto* o = configHandle.getDynamicObject())
                        emitToJs("screenshotSetup", o);
                });

                if (started) return; // emit is deferred to the callback
            }
        }
    }

    // No async restore in flight — emit synchronously as before.
    emitToJs("screenshotSetup", obj);
}

void StellarrBridge::handleScreenshotReady()
{
    // Write signal file so the capture script knows we're ready
    juce::File("/tmp/stellarr-screenshot-ready").create();
}

// -- System stats and tuner ---------------------------------------------------

void StellarrBridge::sendSystemStats(double cpuPercent, float outputPeakLinear)
{
    auto peakDb = outputPeakLinear > 0.0001f
        ? 20.0f * std::log10(outputPeakLinear)
        : -60.0f;

    auto* detail = new juce::DynamicObject();
    detail->setProperty("cpu", cpuPercent);
    detail->setProperty("outputLevelDb", static_cast<double>(peakDb));
    detail->setProperty("clipping", outputPeakLinear > 1.0f);
    emitToJs("systemStats", detail);
}

void StellarrBridge::sendTunerData()
{
    if (processor == nullptr || webView == nullptr) return;

    static const char* noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
            {
                if (!inputBlock->isTunerEnabled()) continue;

                auto noteIdx = inputBlock->getTunerNoteIndex();
                auto* detail = new juce::DynamicObject();
                detail->setProperty("note", noteIdx >= 0 && noteIdx < 12
                    ? juce::String(noteNames[noteIdx]) : juce::String());
                detail->setProperty("octave", inputBlock->getTunerOctave());
                detail->setProperty("cents", static_cast<double>(inputBlock->getTunerCents()));
                detail->setProperty("frequency", static_cast<double>(inputBlock->getTunerFrequency()));
                detail->setProperty("confidence", static_cast<double>(inputBlock->getTunerConfidence()));
                emitToJs("tunerData", detail);
                return;
            }
        }
    }
}

void StellarrBridge::sendMidiMonitorData()
{
    if (processor == nullptr || webView == nullptr) return;

    auto events = processor->getMidiMapper().drainMonitorEvents();
    if (events.empty()) return;

    juce::Array<juce::var> arr;
    for (auto& evt : events)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("type", evt.type);
        obj->setProperty("channel", evt.channel);
        obj->setProperty("data1", evt.data1);
        obj->setProperty("data2", evt.data2);
        arr.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("events", arr);
    emitToJs("midiMonitorData", detail);
}

void StellarrBridge::drainMidiEvents()
{
    if (processor == nullptr) return;
    processor->getMidiMapper().drainOutboundEvents();
}

// -- Plugin management --------------------------------------------------------

void StellarrBridge::handleScanPlugins()
{
    if (processor == nullptr) return;

    emitToJs("scanStarted", new juce::DynamicObject());

    // Run scan on a background thread to avoid freezing the UI.
    // sendPluginList must run on the message thread (bridge emission).
    auto* proc = processor;
    auto* self = this;
    std::thread([proc, self]() {
        proc->getPluginManager().scanPlugins();
        juce::MessageManager::callAsync([self]() {
            self->sendPluginList();
        });
    }).detach();
}

void StellarrBridge::handleGetScanDirectories()
{
    sendScanDirectories();
}

void StellarrBridge::handlePickScanDirectory()
{
    if (processor == nullptr) return;

    juce::MessageManager::callAsync([this]()
    {
        if (processor == nullptr) return;

        juce::FileChooser chooser("Select Plugin Directory");

        if (!chooser.browseForDirectory()) return;

        processor->getPluginManager().addScanDirectory(
            chooser.getResult().getFullPathName());
        sendScanDirectories();
    });
}

void StellarrBridge::handleRemoveScanDirectory(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto path = obj->getProperty("path").toString();
    processor->getPluginManager().removeScanDirectory(path);
    sendScanDirectories();
}

void StellarrBridge::sendPluginList()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> plugins;
    for (auto& desc : processor->getPluginManager().getKnownPlugins().getTypes())
    {
        if (desc.name == "Stellarr") continue;

        auto* p = new juce::DynamicObject();
        p->setProperty("id", desc.createIdentifierString());
        p->setProperty("name", desc.name);
        p->setProperty("manufacturer", desc.manufacturerName);
        p->setProperty("format", desc.pluginFormatName);
        plugins.add(juce::var(p));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("plugins", plugins);
    emitToJs("pluginListUpdated", detail);
}

void StellarrBridge::sendScanDirectories()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> dirs;
    for (auto& d : processor->getPluginManager().getScanDirectories())
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("path", d.path);
        obj->setProperty("isDefault", d.isDefault);
        dirs.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("directories", dirs);
    emitToJs("scanDirectoriesUpdated", detail);
}

// -- Telemetry ----------------------------------------------------------------

void StellarrBridge::handleGetTelemetryEnabled()
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("enabled", stellarr::Telemetry::isEnabled(appProperties));
    emitToJs("telemetryState", detail);
}

void StellarrBridge::handleSetTelemetryEnabled(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    bool enabled = static_cast<bool>(obj->getProperty("enabled"));
    stellarr::Telemetry::setEnabled(appProperties, enabled);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("enabled", enabled);
    emitToJs("telemetryState", detail);
}

// -- Tuner settings -----------------------------------------------------------

void StellarrBridge::handleGetReferencePitch()
{
    float hz = 440.0f;
    if (appProperties != nullptr)
    {
        auto* settings = appProperties->getUserSettings();
        if (settings != nullptr)
            hz = static_cast<float>(settings->getDoubleValue("referencePitch", 440.0));
    }

    // Apply stored value to all input blocks
    if (processor != nullptr)
    {
        for (auto& [blockId, nodeId] : blockNodeMap)
        {
            if (auto* node = processor->getGraph().getNodeForId(nodeId))
                if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
                    inputBlock->setReferencePitch(hz);
        }
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("hz", static_cast<double>(hz));
    emitToJs("referencePitchState", detail);
}

void StellarrBridge::handleSetReferencePitch(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    float hz = static_cast<float>(obj->getProperty("hz"));
    if (hz < 420.0f || hz > 460.0f) return;

    // Persist
    if (appProperties != nullptr)
    {
        auto* settings = appProperties->getUserSettings();
        if (settings != nullptr)
            settings->setValue("referencePitch", static_cast<double>(hz));
    }

    // Apply to all input blocks
    if (processor != nullptr)
    {
        for (auto& [blockId, nodeId] : blockNodeMap)
        {
            if (auto* node = processor->getGraph().getNodeForId(nodeId))
                if (auto* inputBlock = dynamic_cast<stellarr::InputBlock*>(node->getProcessor()))
                    inputBlock->setReferencePitch(hz);
        }
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("hz", static_cast<double>(hz));
    emitToJs("referencePitchState", detail);
}

// -- Loudness metering --------------------------------------------------------

juce::AudioProcessorGraph::Node* StellarrBridge::getNodeForBlockId(const juce::String& blockId)
{
    auto it = blockNodeMap.find(blockId);
    if (it == blockNodeMap.end()) return nullptr;
    return processor->getGraph().getNodeForId(it->second);
}

void StellarrBridge::handleSetSelectedBlock(const juce::var& json)
{
    auto newId = json.getProperty("blockId", "").toString();

    // Disable measurement on previously selected block (unless it's the Output)
    if (selectedBlockId.isNotEmpty() && selectedBlockId != newId)
    {
        if (auto* node = getNodeForBlockId(selectedBlockId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
                if (block->getBlockType() != stellarr::BlockType::output)
                    block->setMeasureLoudness(false);
        }
    }

    selectedBlockId = newId;

    // Enable measurement on the newly selected block
    if (selectedBlockId.isNotEmpty())
    {
        if (auto* node = getNodeForBlockId(selectedBlockId))
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
                block->setMeasureLoudness(true);
    }
}

void StellarrBridge::handleSetTargetLufs(const juce::var& json)
{
    auto blockId = json.getProperty("blockId", "").toString();
    auto value = json.getProperty("lufs", juce::var());

    auto* node = getNodeForBlockId(blockId);
    if (node == nullptr) return;

    auto* output = dynamic_cast<stellarr::OutputBlock*>(node->getProcessor());
    if (output == nullptr) return;

    if (value.isVoid() || value.isUndefined() || value.isString())
        output->setTargetLufs(std::numeric_limits<float>::quiet_NaN());
    else
        output->setTargetLufs(static_cast<float>(static_cast<double>(value)));
}

void StellarrBridge::handleSetLufsWindow(const juce::var& json)
{
    auto window = json.getProperty("window", "shortTerm").toString();
    if (window != "momentary") window = "shortTerm";
    lufsWindow = window;

    if (appProperties != nullptr)
        appProperties->getUserSettings()->setValue("lufsWindow", window);

    auto* detail = new juce::DynamicObject();
    detail->setProperty("window", lufsWindow);
    emitToJs("lufsWindowState", detail);
}

void StellarrBridge::sendBlockMetrics()
{
    if (processor == nullptr) return;

    juce::Array<juce::var> blocksArray;

    for (const auto& [blockId, nodeId] : blockNodeMap)
    {
        auto* node = processor->getGraph().getNodeForId(nodeId);
        if (node == nullptr) continue;
        auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor());
        if (block == nullptr) continue;
        if (! block->isMeasuringLoudness()) continue;

        const float lufs = (lufsWindow == "momentary")
            ? block->getMomentaryLufs()
            : block->getShortTermLufs();

        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", blockId);
        obj->setProperty("lufs", static_cast<double>(lufs));

        if (auto* output = dynamic_cast<stellarr::OutputBlock*>(node->getProcessor()))
        {
            if (output->hasTargetLufs())
                obj->setProperty("targetLufs", static_cast<double>(output->getTargetLufs()));
        }

        blocksArray.add(juce::var(obj));
    }

    auto* detail = new juce::DynamicObject();
    detail->setProperty("blocks", blocksArray);
    detail->setProperty("window", lufsWindow);
    emitToJs("blockMetrics", detail);
}
