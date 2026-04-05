#include "StellarrBridge.h"
#include "StellarrProcessor.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/PluginBlock.h"

StellarrBridge::StellarrBridge() = default;

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
    else if (eventName == "uiReady")                { if (onUiReady) onUiReady(); }

    // Graph
    else if (eventName == "addBlock")               handleAddBlock(json);
    else if (eventName == "removeBlock")            handleRemoveBlock(json);
    else if (eventName == "moveBlock")              handleMoveBlock(json);
    else if (eventName == "addConnection")          handleAddConnection(json);
    else if (eventName == "removeConnection")       handleRemoveConnection(json);
    else if (eventName == "setBlockPlugin")         handleSetBlockPlugin(json);
    else if (eventName == "openPluginEditor")       handleOpenPluginEditor(json);
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
        juce::File samplesDir(STELLARR_SAMPLES_DIR);
        juce::Array<juce::var> files;

        if (samplesDir.isDirectory())
        {
            for (auto& f : samplesDir.findChildFiles(juce::File::findFiles, false, "*.wav"))
                files.add(juce::var(f.getFileNameWithoutExtension()));
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
                    juce::File samplesDir(STELLARR_SAMPLES_DIR);
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
}

// -- Helpers ------------------------------------------------------------------

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

// -- System stats and tuner ---------------------------------------------------

void StellarrBridge::sendSystemStats(double cpuPercent, double memoryMB, double totalMemoryMB)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("cpu", cpuPercent);
    detail->setProperty("memory", memoryMB);
    detail->setProperty("totalMemory", totalMemoryMB);
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

    processor->getPluginManager().scanPlugins();
    sendPluginList();
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
