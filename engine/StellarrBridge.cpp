#include "StellarrBridge.h"
#include "StellarrProcessor.h"
#include "StellarrPlatform.h"
#include "UpdaterShim.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/PluginBlock.h"
#include <cmath>
#include <limits>

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

void StellarrBridge::handleEvent(const juce::String& eventName, const juce::var& payload)
{
    auto json = payload.isString()
        ? juce::JSON::parse(payload.toString())
        : payload;

    // Startup
    if (eventName == "bridgeReady")                 handleBridgeReady();
    else if (eventName == "uiReady")                { if (onUiReady) onUiReady(); handleScreenshotSetup(); }
    else if (eventName == "screenshotReady")          { handleScreenshotReady(); }

    // Software updates (Sparkle)
    else if (eventName == "update/check")            handleUpdateCheck();
    else if (eventName == "update/install")          handleUpdateInstall();
    else if (eventName == "update/open-release-notes") handleUpdateOpenReleaseNotes(json);

    // Graph
    else if (eventName == "addBlock")               handleAddBlock(json);
    else if (eventName == "removeBlock")            handleRemoveBlock(json);
    else if (eventName == "moveBlock")              handleMoveBlock(json);
    else if (eventName == "addConnection")          handleAddConnection(json);
    else if (eventName == "removeConnection")       handleRemoveConnection(json);
    else if (eventName == "setBlockPlugin")         handleSetBlockPlugin(json);
    else if (eventName == "openPluginEditor")       handleOpenPluginEditor(json);
    else if (eventName == "copyBlock")              handleCopyBlock(json);
    else if (eventName == "pasteBlock")             handlePasteBlock(json);
    else if (eventName == "renameBlock" && processor != nullptr)
    {
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
    else if (eventName == "setBlockColor" && processor != nullptr)
    {
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

    // MIDI mappings
    else if (eventName == "addMidiMapping" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        MidiMapper::Mapping m;
        m.channel = static_cast<int>(obj->getProperty("channel"));
        m.ccNumber = static_cast<int>(obj->getProperty("cc"));
        m.target = MidiMapper::targetFromString(obj->getProperty("target").toString());
        m.blockId = obj->getProperty("blockId").toString();
        processor->getMidiMapper().addMapping(m);
        emitMidiMappings();
    }
    else if (eventName == "removeMidiMapping" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        processor->getMidiMapper().removeMapping(static_cast<int>(obj->getProperty("index")));
        emitMidiMappings();
    }
    else if (eventName == "clearMidiMappings" && processor != nullptr)
    {
        processor->getMidiMapper().clearAll();
        emitMidiMappings();
    }
    else if (eventName == "getMidiMappings")
    {
        emitMidiMappings();
    }
    else if (eventName == "startMidiLearn" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto target = MidiMapper::targetFromString(obj->getProperty("target").toString());
        auto blockId = obj->getProperty("blockId").toString();
        processor->getMidiMapper().startLearn(target, blockId);
        emitMidiMappings();
    }
    else if (eventName == "cancelMidiLearn" && processor != nullptr)
    {
        processor->getMidiMapper().cancelLearn();
        emitMidiMappings();
    }
    else if (eventName == "setMidiMonitorEnabled" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;
        processor->getMidiMapper().setMonitorEnabled(static_cast<bool>(obj->getProperty("enabled")));
    }
    else if (eventName == "injectMidiCC" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;
        int ch = static_cast<int>(obj->getProperty("channel")) + 1; // 0-indexed → 1-indexed
        int cc = static_cast<int>(obj->getProperty("cc"));
        int val = static_cast<int>(obj->getProperty("value"));
        processor->getMidiMapper().injectMidi(juce::MidiMessage::controllerEvent(ch, cc, val));
    }

    // Plugin management
    else if (eventName == "scanPlugins")            handleScanPlugins();
    else if (eventName == "getScanDirectories")     handleGetScanDirectories();
    else if (eventName == "pickScanDirectory")      handlePickScanDirectory();
    else if (eventName == "removeScanDirectory")    handleRemoveScanDirectory(json);

    // Telemetry
    else if (eventName == "getTelemetryEnabled")    handleGetTelemetryEnabled();
    else if (eventName == "setTelemetryEnabled")    handleSetTelemetryEnabled(json);

    // Tuner settings
    else if (eventName == "getReferencePitch")      handleGetReferencePitch();
    else if (eventName == "setReferencePitch")      handleSetReferencePitch(json);

    // Presets
    else if (eventName == "newSession")             handleNewSession();
    else if (eventName == "saveSession")            handleSaveSession();
    else if (eventName == "saveSessionQuiet")       handleSaveSessionQuiet();
    else if (eventName == "loadSession")            handleLoadSession();
    else if (eventName == "pickPresetDirectory")    handlePickPresetDirectory();
    else if (eventName == "loadPresetByIndex")      handleLoadPresetByIndex(json);
    else if (eventName == "renamePreset")           handleRenamePreset(json);
    else if (eventName == "deletePreset")           handleDeletePreset(json);
    else if (eventName == "getPresetList")          handleGetPresetList();
    else if (eventName == "setGridSize")            handleSetGridSize(json);

    // Scenes
    else if (eventName == "addScene")               handleAddScene();
    else if (eventName == "recallScene")            handleRecallScene(json);
    else if (eventName == "saveScene")              handleSaveScene(json);
    else if (eventName == "renameScene")            handleRenameScene(json);
    else if (eventName == "deleteScene")            handleDeleteScene(json);

    // Input block controls
    else if (eventName == "toggleTestTone" && processor != nullptr)
    {
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
    else if (eventName == "getTestToneSamples")
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
    else if (eventName == "setTestToneSample" && processor != nullptr)
    {
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
    else if (eventName == "setTunerEnabled" && processor != nullptr)
    {
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

    // Block parameters (DRY via handleSetBlockParam)
    else if (eventName == "setBlockMix")
        handleSetBlockParam(json, "mix",
            [](stellarr::Block* b, const juce::var& v) { b->setMix(static_cast<float>(v)); },
            "blockMixChanged",
            [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getMix())); });
    else if (eventName == "setBlockBalance")
        handleSetBlockParam(json, "balance",
            [](stellarr::Block* b, const juce::var& v) { b->setBalance(static_cast<float>(v)); },
            "blockBalanceChanged",
            [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getBalance())); });
    else if (eventName == "setBlockLevel")
        handleSetBlockParam(json, "level",
            [](stellarr::Block* b, const juce::var& v) { b->setLevelDb(static_cast<float>(v)); },
            "blockLevelChanged",
            [](stellarr::Block* b) { return juce::var(static_cast<double>(b->getLevelDb())); });
    else if (eventName == "toggleBlockBypass" && processor != nullptr)
    {
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
    else if (eventName == "setBlockBypassMode")
        handleSetBlockParam(json, "bypassMode",
            [](stellarr::Block* b, const juce::var& v) {
                b->setBypassMode(stellarr::bypassModeFromString(v.toString()));
            },
            "blockBypassModeChanged",
            [](stellarr::Block* b) {
                return juce::var(stellarr::bypassModeToString(b->getBypassMode()));
            });

    // Block states
    else if (eventName == "saveBlockState")          handleBlockStateEvent(json, "save");
    else if (eventName == "addBlockState")           handleBlockStateEvent(json, "add");
    else if (eventName == "recallBlockState")        handleBlockStateEvent(json, "recall");
    else if (eventName == "deleteBlockState")        handleBlockStateEvent(json, "delete");

    // Loudness metering
    else if (eventName == "setSelectedBlock")        handleSetSelectedBlock(json);
    else if (eventName == "setTargetLufs")           handleSetTargetLufs(json);
    else if (eventName == "setLufsWindow")           handleSetLufsWindow(json);
}

// -- Helpers ------------------------------------------------------------------

void StellarrBridge::connectIOBlock(const juce::String& type,
                                    juce::AudioProcessorGraph::NodeID nodeId,
                                    juce::AudioProcessorGraph::UpdateKind update)
{
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
    if (webView == nullptr) return;

    auto eventId = juce::Identifier(eventName);
    auto data = juce::var(detail);

    juce::MessageManager::callAsync([this, eventId, data]()
    {
        if (webView != nullptr)
            webView->emitEventIfBrowserIsVisible(eventId, data);
    });
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

                bool restored = false;

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
                                restoreSession(session);
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

                                restored = true;
                            }
                        }
                    }
                }

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

    auto screenshotConfig = juce::JSON::parse(configFile.loadFileAsString());
    configFile.deleteFile();

    auto* obj = screenshotConfig.getDynamicObject();
    if (obj == nullptr) return;

    // Load preset if specified
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
                restoreSession(session);
                sendGraphState();

                // Recall scene if specified
                if (obj->hasProperty("scene"))
                {
                    auto sceneJson = juce::JSON::parse(
                        "{\"index\":" + juce::String(static_cast<int>(obj->getProperty("scene"))) + "}");
                    handleRecallScene(sceneJson);
                }
            }
        }
    }

    // Send config to UI for page navigation and actions
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
