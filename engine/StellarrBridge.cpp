#include "StellarrBridge.h"
#include "StellarrProcessor.h"
#include "blocks/InputBlock.h"
#include "blocks/OutputBlock.h"
#include "blocks/PluginBlock.h"

StellarrBridge::StellarrBridge() = default;

void StellarrBridge::setProcessor(StellarrProcessor* proc)
{
    processor = proc;
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

void StellarrBridge::handleEvent(const juce::String& eventName, const juce::var& payload)
{
    auto json = payload.isString()
        ? juce::JSON::parse(payload.toString())
        : payload;

    if (eventName == "bridgeReady")
        handleBridgeReady();
    else if (eventName == "addBlock")            handleAddBlock(json);
    else if (eventName == "removeBlock")         handleRemoveBlock(json);
    else if (eventName == "moveBlock")           handleMoveBlock(json);
    else if (eventName == "addConnection")       handleAddConnection(json);
    else if (eventName == "removeConnection")    handleRemoveConnection(json);
    else if (eventName == "setBlockPlugin")      handleSetBlockPlugin(json);
    else if (eventName == "openPluginEditor")    handleOpenPluginEditor(json);
    else if (eventName == "scanPlugins")         handleScanPlugins();
    else if (eventName == "getScanDirectories")  handleGetScanDirectories();
    else if (eventName == "pickScanDirectory")   handlePickScanDirectory();
    else if (eventName == "removeScanDirectory") handleRemoveScanDirectory(json);
    else if (eventName == "newSession")           handleNewSession();
    else if (eventName == "saveSession")         handleSaveSession();
    else if (eventName == "saveSessionQuiet")    handleSaveSessionQuiet();
    else if (eventName == "loadSession")         handleLoadSession();
    else if (eventName == "pickPresetDirectory") handlePickPresetDirectory();
    else if (eventName == "loadPresetByIndex")   handleLoadPresetByIndex(json);
    else if (eventName == "getPresetList")       handleGetPresetList();
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
    else if (eventName == "setBlockMix" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto mixVal  = static_cast<float>(obj->getProperty("mix"));

        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                block->setMix(mixVal);
                if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                {
                    pb->markDirty();
                    emitBlockStates(blockId, pb);
                }

                auto* detail = new juce::DynamicObject();
                detail->setProperty("blockId", blockId);
                detail->setProperty("mix", static_cast<double>(mixVal));
                emitToJs("blockMixChanged", detail);
            }
        }
    }
    else if (eventName == "setBlockBalance" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId  = obj->getProperty("blockId").toString();
        auto balVal   = static_cast<float>(obj->getProperty("balance"));

        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                block->setBalance(balVal);
                if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                {
                    pb->markDirty();
                    emitBlockStates(blockId, pb);
                }

                auto* detail = new juce::DynamicObject();
                detail->setProperty("blockId", blockId);
                detail->setProperty("balance", static_cast<double>(balVal));
                emitToJs("blockBalanceChanged", detail);
            }
        }
    }
    else if (eventName == "toggleBlockBypass" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();

        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                bool newState = !block->isBypassed();
                block->setBypassed(newState);
                if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                {
                    pb->markDirty();
                    emitBlockStates(blockId, pb);
                }

                auto* detail = new juce::DynamicObject();
                detail->setProperty("blockId", blockId);
                detail->setProperty("bypassed", newState);
                emitToJs("blockBypassChanged", detail);
            }
        }
    }
    else if (eventName == "setBlockBypassMode" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto modeStr = obj->getProperty("mode").toString();

        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                block->setBypassMode(stellarr::bypassModeFromString(modeStr));
                if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                {
                    pb->markDirty();
                    emitBlockStates(blockId, pb);
                }

                auto* detail = new juce::DynamicObject();
                detail->setProperty("blockId", blockId);
                detail->setProperty("bypassMode", modeStr);
                emitToJs("blockBypassModeChanged", detail);
            }
        }
    }
    else if (eventName == "saveBlockState" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                pluginBlock->saveCurrentState();
                emitBlockStates(blockId, pluginBlock);
            }
        }
    }
    else if (eventName == "addBlockState" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                pluginBlock->addState();
                emitBlockStates(blockId, pluginBlock);
            }
        }
    }
    else if (eventName == "recallBlockState" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto index = static_cast<int>(obj->getProperty("index"));
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                if (pluginBlock->recallState(index))
                {
                    emitBlockStates(blockId, pluginBlock);
                    // Emit param changes so UI sliders update
                    emitBlockParams(blockId, pluginBlock);
                }
            }
        }
    }
    else if (eventName == "deleteBlockState" && processor != nullptr)
    {
        auto* obj = json.getDynamicObject();
        if (obj == nullptr) return;

        auto blockId = obj->getProperty("blockId").toString();
        auto index = static_cast<int>(obj->getProperty("index"));
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) return;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                if (pluginBlock->deleteState(index))
                {
                    emitBlockStates(blockId, pluginBlock);
                    emitBlockParams(blockId, pluginBlock);
                }
            }
        }
    }
    else if (eventName == "addScene")              handleAddScene();
    else if (eventName == "recallScene")           handleRecallScene(json);
    else if (eventName == "saveScene")             handleSaveScene(json);
    else if (eventName == "renameScene")           handleRenameScene(json);
    else if (eventName == "deleteScene")           handleDeleteScene(json);
    else if (eventName == "uiAction" && webView != nullptr)
    {
        auto* response = new juce::DynamicObject();
        response->setProperty("message", "Pong: " + payload.toString());
        webView->emitEventIfBrowserIsVisible("pong", juce::var(response));
    }
}

void StellarrBridge::emitToJs(const juce::String& eventName, juce::DynamicObject* detail)
{
    if (webView == nullptr) return;

    // Defer emission so it runs after the native function callback returns.
    // emitEventIfBrowserIsVisible may not deliver events re-entrantly
    // from within a WKWebView message handler.
    auto eventId = juce::Identifier(eventName);
    auto data = juce::var(detail);

    juce::MessageManager::callAsync([this, eventId, data]()
    {
        if (webView != nullptr)
            webView->emitEventIfBrowserIsVisible(eventId, data);
    });
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

// -- Scene management ---------------------------------------------------------

void StellarrBridge::emitScenes()
{
    auto* detail = new juce::DynamicObject();

    juce::Array<juce::var> arr;
    for (auto& scene : scenes)
    {
        auto* so = new juce::DynamicObject();
        so->setProperty("name", scene.name);
        auto* mapObj = new juce::DynamicObject();
        for (auto& [bid, si] : scene.blockStateMap)
            mapObj->setProperty(bid, si);
        so->setProperty("blockStateMap", juce::var(mapObj));
        arr.add(juce::var(so));
    }
    detail->setProperty("scenes", arr);
    detail->setProperty("activeSceneIndex", activeSceneIndex);
    emitToJs("scenesChanged", detail);
}

struct SceneCapture {
    std::map<juce::String, int> stateMap;
    std::map<juce::String, bool> bypassMap;
};

static SceneCapture StellarrBridge_captureScene(
    const std::map<juce::String, juce::AudioProcessorGraph::NodeID>& blockNodeMap,
    juce::AudioProcessorGraph& graph)
{
    SceneCapture cap;
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = graph.getNodeForId(nodeId))
        {
            if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                cap.stateMap[blockId] = pb->getActiveStateIndex();
                cap.bypassMap[blockId] = pb->isBypassed();
            }
        }
    }
    return cap;
}

static void captureIntoScene(StellarrBridge::Scene& scene,
                             const std::map<juce::String, juce::AudioProcessorGraph::NodeID>& blockNodeMap,
                             juce::AudioProcessorGraph& graph)
{
    auto cap = StellarrBridge_captureScene(blockNodeMap, graph);
    scene.blockStateMap = cap.stateMap;
    scene.blockBypassMap = cap.bypassMap;
}

void StellarrBridge::handleAddScene()
{
    if (processor == nullptr || static_cast<int>(scenes.size()) >= maxScenes) return;

    // Update outgoing scene before adding new one
    if (activeSceneIndex >= 0 && activeSceneIndex < static_cast<int>(scenes.size()))
        captureIntoScene(scenes[static_cast<size_t>(activeSceneIndex)], blockNodeMap, processor->getGraph());

    Scene scene;
    scene.name = "Scene " + juce::String(static_cast<int>(scenes.size()) + 1);
    captureIntoScene(scene, blockNodeMap, processor->getGraph());

    scenes.push_back(scene);
    activeSceneIndex = static_cast<int>(scenes.size()) - 1;
    emitScenes();
}

void StellarrBridge::handleRecallScene(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= static_cast<int>(scenes.size())) return;

    // Save current live states and update outgoing scene's block map
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
            if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                pb->saveCurrentState();
    }

    if (activeSceneIndex >= 0 && activeSceneIndex < static_cast<int>(scenes.size()))
        captureIntoScene(scenes[static_cast<size_t>(activeSceneIndex)], blockNodeMap, processor->getGraph());

    activeSceneIndex = index;
    auto& scene = scenes[static_cast<size_t>(index)];

    for (auto& [blockId, stateIdx] : scene.blockStateMap)
    {
        auto nodeIt = blockNodeMap.find(blockId);
        if (nodeIt == blockNodeMap.end()) continue;

        if (auto* node = processor->getGraph().getNodeForId(nodeIt->second))
        {
            if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                int clampedIdx = std::min(stateIdx, pb->getNumStates() - 1);
                if (clampedIdx >= 0)
                {
                    pb->recallState(clampedIdx);

                    // Restore bypass from scene
                    auto bypassIt = scene.blockBypassMap.find(blockId);
                    if (bypassIt != scene.blockBypassMap.end())
                        pb->setBypassed(bypassIt->second);

                    emitBlockStates(blockId, pb);
                    emitBlockParams(blockId, pb);
                }
            }
        }
    }

    emitScenes();
}

void StellarrBridge::handleSaveScene(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= static_cast<int>(scenes.size())) return;

    captureIntoScene(scenes[static_cast<size_t>(index)], blockNodeMap, processor->getGraph());
    emitScenes();
}

void StellarrBridge::handleRenameScene(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    auto name = obj->getProperty("name").toString();
    if (index < 0 || index >= static_cast<int>(scenes.size())) return;

    scenes[static_cast<size_t>(index)].name = name;
    emitScenes();
}

void StellarrBridge::handleDeleteScene(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= static_cast<int>(scenes.size()) || scenes.size() <= 1) return;

    scenes.erase(scenes.begin() + index);

    if (activeSceneIndex >= static_cast<int>(scenes.size()))
        activeSceneIndex = static_cast<int>(scenes.size()) - 1;
    else if (index < activeSceneIndex)
        activeSceneIndex--;

    emitScenes();
}

// -- Block state helpers ------------------------------------------------------

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

    auto* bypDetail = new juce::DynamicObject();
    bypDetail->setProperty("blockId", blockId);
    bypDetail->setProperty("bypassed", block->isBypassed());
    emitToJs("blockBypassChanged", bypDetail);

    auto* modeDetail = new juce::DynamicObject();
    modeDetail->setProperty("blockId", blockId);
    modeDetail->setProperty("bypassMode", stellarr::bypassModeToString(block->getBypassMode()));
    emitToJs("blockBypassModeChanged", modeDetail);
}

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

                    // Restore preset directory and index
                    auto savedDir = settings->getValue("lastPresetDirectory", "");
                    auto savedIndex = settings->getIntValue("lastPresetIndex", -1);
                    if (savedDir.isNotEmpty())
                    {
                        presetDirectory = juce::File(savedDir);
                        handleGetPresetList();
                        currentPresetIndex = savedIndex;
                    }

                    // Restore last loaded/saved preset file
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

                // Default: create Input and Output blocks
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
                if (onStartupComplete)
                    onStartupComplete();
            });
        });
    });
}

// -- Graph event handlers ----------------------------------------------------

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

    // Auto-wire user I/O blocks to the graph's built-in audio I/O nodes.
    // Remove the default built-in pass-through so it does not compete.
    if (type == "input")
    {
        processor->disconnectBlocks(processor->getAudioInputNodeId(), processor->getAudioOutputNodeId());
        processor->connectBlocks(processor->getAudioInputNodeId(), nodeId);
    }
    else if (type == "output")
    {
        processor->disconnectBlocks(processor->getAudioInputNodeId(), processor->getAudioOutputNodeId());
        processor->connectBlocks(nodeId, processor->getAudioOutputNodeId());
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

            // Notify UI of the new connections
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

    // Remove all connections involving this block
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

    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    auto* node = processor->getGraph().getNodeForId(nodeIt->second);
    if (node == nullptr) return;

    auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
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

void StellarrBridge::handleOpenPluginEditor(const juce::var& json)
{
    if (processor == nullptr) return;

    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto blockId = obj->getProperty("blockId").toString();
    auto nodeIt = blockNodeMap.find(blockId);
    if (nodeIt == blockNodeMap.end()) return;

    auto* node = processor->getGraph().getNodeForId(nodeIt->second);
    if (node == nullptr) return;

    auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
    if (pluginBlock == nullptr || !pluginBlock->hasPlugin()) return;

    // Defer to message thread in case called from within native function handler
    juce::MessageManager::callAsync([pluginBlock]()
    {
        pluginBlock->openPluginEditor();
    });
}

void StellarrBridge::sendSystemStats(double cpuPercent, double memoryMB, double totalMemoryMB)
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("cpu", cpuPercent);
    detail->setProperty("memory", memoryMB);
    detail->setProperty("totalMemory", totalMemoryMB);
    emitToJs("systemStats", detail);
}

// -- State persistence -------------------------------------------------------

void StellarrBridge::setPresetFromFile(const juce::File& file)
{
    lastPresetFile = file;
    presetDirectory = file.getParentDirectory();
    handleGetPresetList();

    currentPresetIndex = -1;
    for (int i = 0; i < presetFiles.size(); ++i)
    {
        if (presetFiles[i] == file.getFileName())
        {
            currentPresetIndex = i;
            break;
        }
    }

    sendPresetList();
    persistPresetInfo();
}

void StellarrBridge::persistPresetInfo()
{
    if (appProperties == nullptr) return;

    auto* settings = appProperties->getUserSettings();
    settings->setValue("lastPresetDirectory", presetDirectory.getFullPathName());
    settings->setValue("lastPresetIndex", currentPresetIndex);
    settings->setValue("lastPresetFile", lastPresetFile.getFullPathName());
    appProperties->saveIfNeeded();
}

// -- Plugin management -------------------------------------------------------

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

// -- Welcome and graph state -------------------------------------------------

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
                blockObj->setProperty("bypassed", block->isBypassed());
                blockObj->setProperty("bypassMode", stellarr::bypassModeToString(block->getBypassMode()));
            }
        }

        blocksArray.add(juce::var(blockObj));
    }

    juce::Array<juce::var> connectionsArray;
    // Build reverse map: nodeID → blockID
    std::map<juce::uint32, juce::String> nodeToBlock;
    for (auto& [blockId, nodeId] : blockNodeMap)
        nodeToBlock[nodeId.uid] = blockId;

    for (auto& conn : processor->getGraph().getConnections())
    {
        if (conn.source.channelIndex != 0) continue; // Only emit once per connection pair

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

// -- Session serialisation ---------------------------------------------------

juce::var StellarrBridge::serialiseSession() const
{
    if (processor == nullptr) return {};

    auto* session = new juce::DynamicObject();
    session->setProperty("version", 1);

    // Grid
    auto* gridObj = new juce::DynamicObject();
    gridObj->setProperty("columns", 12);
    gridObj->setProperty("rows", 6);
    session->setProperty("grid", juce::var(gridObj));

    // Blocks
    juce::Array<juce::var> blocksArray;
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
        {
            if (auto* block = dynamic_cast<stellarr::Block*>(node->getProcessor()))
            {
                auto blockJson = block->toJson();
                if (auto* obj = blockJson.getDynamicObject())
                {
                    auto posIt = blockPositions.find(blockId);
                    if (posIt != blockPositions.end())
                    {
                        obj->setProperty("col", posIt->second.first);
                        obj->setProperty("row", posIt->second.second);
                    }
                }
                blocksArray.add(blockJson);
            }
        }
    }
    session->setProperty("blocks", blocksArray);

    // Connections
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
    session->setProperty("connections", connectionsArray);

    // Update active scene before serialising
    if (activeSceneIndex >= 0 && activeSceneIndex < static_cast<int>(scenes.size()))
        captureIntoScene(const_cast<StellarrBridge*>(this)->scenes[static_cast<size_t>(activeSceneIndex)],
                         blockNodeMap, processor->getGraph());

    // Scenes
    juce::Array<juce::var> scenesArray;
    for (auto& scene : scenes)
    {
        auto* sceneObj = new juce::DynamicObject();
        sceneObj->setProperty("name", scene.name);
        auto* mapObj = new juce::DynamicObject();
        for (auto& [bid, si] : scene.blockStateMap)
            mapObj->setProperty(bid, si);
        sceneObj->setProperty("blockStateMap", juce::var(mapObj));
        auto* bypassObj = new juce::DynamicObject();
        for (auto& [bid, bp] : scene.blockBypassMap)
            bypassObj->setProperty(bid, bp);
        sceneObj->setProperty("blockBypassMap", juce::var(bypassObj));
        scenesArray.add(juce::var(sceneObj));
    }
    session->setProperty("scenes", scenesArray);
    session->setProperty("activeSceneIndex", activeSceneIndex);

    return juce::var(session);
}

void StellarrBridge::clearGraph()
{
    // Close all plugin editor windows
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                pluginBlock->closePluginEditor();
    }

    // Remove all user blocks
    auto ids = blockNodeMap;
    for (auto& [blockId, nodeId] : ids)
        processor->removeBlock(nodeId);

    blockNodeMap.clear();
    blockPositions.clear();
}

void StellarrBridge::restoreSession(const juce::var& session)
{
    if (processor == nullptr) return;

    auto* obj = session.getDynamicObject();
    if (obj == nullptr) return;

    clearGraph();

    // Restore blocks
    auto blocksVar = obj->getProperty("blocks");
    if (auto* blocksArray = blocksVar.getArray())
    {
        for (auto& blockVar : *blocksArray)
        {
            auto* blockObj = blockVar.getDynamicObject();
            if (blockObj == nullptr) continue;

            auto type = blockObj->getProperty("type").toString();
            auto col  = static_cast<int>(blockObj->getProperty("col"));
            auto row  = static_cast<int>(blockObj->getProperty("row"));
            auto savedId = blockObj->getProperty("id").toString();

            std::unique_ptr<stellarr::Block> block;
            if (type == "input")       block = std::make_unique<stellarr::InputBlock>();
            else if (type == "output") block = std::make_unique<stellarr::OutputBlock>();
            else if (type == "plugin" || type == "vst")  block = std::make_unique<stellarr::PluginBlock>();
            else continue;

            block->fromJson(blockVar);
            block->resetToDefault();

            auto blockId = savedId.isNotEmpty() ? savedId : block->getBlockId().toString();
            auto nodeId = processor->addBlock(std::move(block));
            if (nodeId.uid == 0) continue;

            blockNodeMap[blockId] = nodeId;
            blockPositions[blockId] = {col, row};

            // Auto-wire I/O blocks
            if (type == "input")
                processor->connectBlocks(processor->getAudioInputNodeId(), nodeId);
            else if (type == "output")
                processor->connectBlocks(nodeId, processor->getAudioOutputNodeId());

            // Restore plugin
            if (type == "plugin" || type == "vst")
            {
                auto pluginId = blockObj->getProperty("pluginId").toString();
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
        }
    }

    // Restore connections
    auto connectionsVar = obj->getProperty("connections");
    if (auto* connectionsArray = connectionsVar.getArray())
    {
        for (auto& connVar : *connectionsArray)
        {
            auto* connObj = connVar.getDynamicObject();
            if (connObj == nullptr) continue;

            auto sourceId = connObj->getProperty("sourceId").toString();
            auto destId   = connObj->getProperty("destId").toString();

            auto srcIt = blockNodeMap.find(sourceId);
            auto dstIt = blockNodeMap.find(destId);
            if (srcIt == blockNodeMap.end() || dstIt == blockNodeMap.end()) continue;

            processor->connectBlocks(srcIt->second, dstIt->second);
        }
    }

    // Restore scenes
    scenes.clear();
    activeSceneIndex = -1;
    auto scenesVar = obj->getProperty("scenes");
    if (auto* scenesArr = scenesVar.getArray())
    {
        for (auto& sv : *scenesArr)
        {
            if (auto* so = sv.getDynamicObject())
            {
                Scene scene;
                scene.name = so->getProperty("name").toString();
                auto mapVar = so->getProperty("blockStateMap");
                if (auto* mapObj = mapVar.getDynamicObject())
                {
                    for (auto& prop : mapObj->getProperties())
                        scene.blockStateMap[prop.name.toString()] = static_cast<int>(prop.value);
                }
                auto bypassVar = so->getProperty("blockBypassMap");
                if (auto* bypassObj = bypassVar.getDynamicObject())
                {
                    for (auto& prop : bypassObj->getProperties())
                        scene.blockBypassMap[prop.name.toString()] = static_cast<bool>(prop.value);
                }
                scenes.push_back(scene);
            }
        }
        activeSceneIndex = static_cast<int>(obj->getProperty("activeSceneIndex"));
        if (activeSceneIndex >= static_cast<int>(scenes.size()))
            activeSceneIndex = scenes.empty() ? -1 : 0;
    }

    // Ensure at least one scene exists
    if (scenes.empty())
    {
        Scene defaultScene;
        defaultScene.name = "Scene 1";
        captureIntoScene(defaultScene, blockNodeMap, processor->getGraph());
        scenes.push_back(defaultScene);
        activeSceneIndex = 0;
    }

    sendGraphState();
}

// -- Preset management -------------------------------------------------------

void StellarrBridge::handleNewSession()
{
    clearGraph();

    // Create default Input and Output blocks
    auto inputJson = juce::JSON::parse(R"({"type":"input","col":0,"row":2})");
    auto outputJson = juce::JSON::parse(R"({"type":"output","col":11,"row":2})");
    handleAddBlock(inputJson);
    handleAddBlock(outputJson);

    // Clear preset info and create default scene
    lastPresetFile = juce::File{};
    currentPresetIndex = -1;
    scenes.clear();
    Scene defaultScene;
    defaultScene.name = "Scene 1";
    {
        auto cap = StellarrBridge_captureScene(blockNodeMap, processor->getGraph());
        defaultScene.blockStateMap = cap.stateMap;
        defaultScene.blockBypassMap = cap.bypassMap;
    }
    scenes.push_back(defaultScene);
    activeSceneIndex = 0;
    persistPresetInfo();

    sendGraphState();
    sendPresetList();
}

void StellarrBridge::handleSaveSession()
{
    if (processor == nullptr) return;

    juce::MessageManager::callAsync([this]()
    {
        juce::FileChooser chooser("Save Preset", presetDirectory, "*.stellarr");

        if (!chooser.browseForFileToSave(true)) return;

        auto file = chooser.getResult().withFileExtension("stellarr");
        auto session = serialiseSession();
        auto jsonStr = juce::JSON::toString(session);
        file.replaceWithText(jsonStr);

        setPresetFromFile(file);
        clearAllDirtyStates();
        emitToJs("sessionSaved", new juce::DynamicObject());
    });
}

void StellarrBridge::handleSaveSessionQuiet()
{
    if (processor == nullptr) return;

    if (lastPresetFile.existsAsFile())
    {
        auto session = serialiseSession();
        auto jsonStr = juce::JSON::toString(session);
        lastPresetFile.replaceWithText(jsonStr);

        clearAllDirtyStates();
        emitToJs("sessionSaved", new juce::DynamicObject());
    }
    else
    {
        handleSaveSession();
    }
}

void StellarrBridge::handleLoadSession()
{
    if (processor == nullptr) return;

    juce::MessageManager::callAsync([this]()
    {
        juce::FileChooser chooser("Load Preset", presetDirectory, "*.stellarr");

        if (!chooser.browseForFileToOpen()) return;

        auto file = chooser.getResult();
        auto jsonStr = file.loadFileAsString();
        auto session = juce::JSON::parse(jsonStr);
        restoreSession(session);
        setPresetFromFile(file);
    });
}

void StellarrBridge::handlePickPresetDirectory()
{
    juce::MessageManager::callAsync([this]()
    {
        juce::FileChooser chooser("Select Preset Directory");

        if (!chooser.browseForDirectory()) return;

        presetDirectory = chooser.getResult();
        currentPresetIndex = -1;
        handleGetPresetList();
        sendPresetList();
        persistPresetInfo();
    });
}

void StellarrBridge::handleGetPresetList()
{
    presetFiles.clear();

    if (presetDirectory.isDirectory())
    {
        for (auto& f : presetDirectory.findChildFiles(
                juce::File::findFiles, false, "*.stellarr"))
        {
            presetFiles.add(f.getFileName());
        }

        presetFiles.sort(true);
    }
}

void StellarrBridge::handleLoadPresetByIndex(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= presetFiles.size()) return;

    currentPresetIndex = index;
    auto file = presetDirectory.getChildFile(presetFiles[index]);
    auto jsonStr = file.loadFileAsString();
    auto session = juce::JSON::parse(jsonStr);
    restoreSession(session);
    setPresetFromFile(file);
}

void StellarrBridge::sendPresetList()
{
    juce::Array<juce::var> files;
    for (auto& f : presetFiles)
        files.add(juce::var(f));

    auto* detail = new juce::DynamicObject();
    detail->setProperty("directory", presetDirectory.getFullPathName());
    detail->setProperty("files", files);
    detail->setProperty("currentIndex", currentPresetIndex);
    emitToJs("presetListUpdated", detail);
}
