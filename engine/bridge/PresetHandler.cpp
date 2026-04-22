#include "../StellarrProcessor.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include "SceneCapture.h"

// -- Session serialization ----------------------------------------------------

juce::var StellarrBridge::serialiseSession() const
{
    if (processor == nullptr) return {};

    auto* session = new juce::DynamicObject();
    session->setProperty("version", 1);

    auto* gridObj = new juce::DynamicObject();
    gridObj->setProperty("columns", gridCols);
    gridObj->setProperty("rows", gridRows);
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

    // MIDI mappings (preset-level only — global mappings stored in app settings)
    if (processor != nullptr)
        session->setProperty("midiMappings", processor->getMidiMapper().presetMappingsToJson());

    return juce::var(session);
}

void StellarrBridge::clearGraph()
{
    using UK = juce::AudioProcessorGraph::UpdateKind;

    // Close plugin editor windows first — removeBlock deletes the processor,
    // which would leave dangling window references.
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                pluginBlock->closePluginEditor();
    }

    // Remove all blocks without rebuilding after each one.
    auto ids = blockNodeMap;
    for (auto& [blockId, nodeId] : ids)
        processor->removeBlock(nodeId, UK::none);

    blockNodeMap.clear();
    blockPositions.clear();
}

void StellarrBridge::restoreSession(const juce::var& session)
{
    if (processor == nullptr) return;

    auto* obj = session.getDynamicObject();
    if (obj == nullptr) return;

    using UK = juce::AudioProcessorGraph::UpdateKind;

    // -- Phase 1: Pre-create plugin instances while audio is still running -----
    // This is the slow part (loading plugin binaries from disk). We do it before
    // suspending so the audio gap is as short as possible.

    struct PluginPreload {
        juce::String blockId;
        juce::String pluginId;
        juce::String pluginName;
        std::unique_ptr<juce::AudioPluginInstance> instance;
    };
    std::vector<PluginPreload> preloads;

    auto blocksVar = obj->getProperty("blocks");
    if (auto* blocksArray = blocksVar.getArray())
    {
        for (auto& blockVar : *blocksArray)
        {
            auto* blockObj = blockVar.getDynamicObject();
            if (blockObj == nullptr) continue;

            auto type = blockObj->getProperty("type").toString();
            if (type != "plugin" && type != "vst") continue;

            auto pluginId = blockObj->getProperty("pluginId").toString();
            if (pluginId.isEmpty()) continue;

            auto savedId = blockObj->getProperty("id").toString();
            auto pluginName = blockObj->getProperty("pluginName").toString();

            juce::String errorMessage;
            auto instance = processor->getPluginManager().createPluginInstance(
                pluginId, processor->getSampleRate(),
                processor->getBlockSize(), errorMessage);

            if (instance != nullptr)
            {
                instance->setPlayConfigDetails(2, 2, processor->getSampleRate(),
                                               processor->getBlockSize());
                instance->prepareToPlay(processor->getSampleRate(),
                                        processor->getBlockSize());
            }

            preloads.push_back({savedId, pluginId, pluginName, std::move(instance)});
        }
    }

    // -- Phase 2: Suspend and rebuild the graph atomically --------------------
    // Now that all plugins are pre-loaded, the suspension window is minimal:
    // just pointer swaps, connection wiring, and a single graph rebuild.

    processor->suspendProcessing(true);

    clearGraph();

    // Restore blocks (all graph mutations use UpdateKind::none — no intermediate rebuilds)
    if (auto* blocksArray = blocksVar.getArray())
    {
        // Build a lookup from saved block ID to preloaded instance
        std::map<juce::String, size_t> preloadIndex;
        for (size_t i = 0; i < preloads.size(); ++i)
            preloadIndex[preloads[i].blockId] = i;

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
            auto nodeId = processor->addBlock(std::move(block), UK::none);
            if (nodeId.uid == 0) continue;

            blockNodeMap[blockId] = nodeId;
            blockPositions[blockId] = {col, row};

            connectIOBlock(type, nodeId, UK::none);

            // Install pre-loaded plugin instance (just a pointer swap, fast)
            if (type == "plugin" || type == "vst")
            {
                auto preloadIt = preloadIndex.find(blockId);
                if (preloadIt != preloadIndex.end())
                {
                    auto& pl = preloads[preloadIt->second];
                    if (auto* node = processor->getGraph().getNodeForId(nodeId))
                    {
                        if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                        {
                            if (pl.instance != nullptr)
                            {
                                pb->setPlugin(std::move(pl.instance), pl.pluginId);
                                pb->restorePluginState();
                            }
                            else
                            {
                                pb->setPluginMissing(true);
                                pb->setMissingPluginName(pl.pluginName);
                            }
                        }
                    }
                }
            }
        }
    }

    // Restore connections (also batched)
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

            processor->connectBlocks(srcIt->second, dstIt->second, 2, UK::none);
        }
    }

    // Single atomic rebuild — audio thread picks up the complete new graph in one swap
    processor->rebuildGraph();

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

    // Restore preset-level MIDI mappings (always clear old, keeps global intact)
    processor->getMidiMapper().loadPresetMappings(
        obj->hasProperty("midiMappings") ? obj->getProperty("midiMappings") : juce::var());
    emitMidiMappings();

    // Resume graph-level routing
    processor->suspendProcessing(false);

    // Restore grid dimensions (falls back to current defaults if absent)
    if (obj->hasProperty("grid"))
    {
        if (auto* gridObj = obj->getProperty("grid").getDynamicObject())
        {
            auto cols = gridObj->getProperty("columns");
            auto rows = gridObj->getProperty("rows");
            if (cols.isInt() || cols.isInt64() || cols.isDouble())
                gridCols = static_cast<int>(cols);
            if (rows.isInt() || rows.isInt64() || rows.isDouble())
                gridRows = static_cast<int>(rows);
        }
    }

    sendGraphState();
    emitGridState();
}

// -- Preset management --------------------------------------------------------

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

    // Persist global MIDI mappings (preset change, tuner toggle)
    if (processor != nullptr)
    {
        auto globalJson = juce::JSON::toString(processor->getMidiMapper().globalMappingsToJson());
        settings->setValue("globalMidiMappings", globalJson);
    }

    appProperties->saveIfNeeded();
}

void StellarrBridge::handleNewSession()
{
    clearGraph();

    auto inputJson = juce::JSON::parse(R"({"type":"input","col":0,"row":2})");
    auto outputJson = juce::JSON::parse(R"({"type":"output","col":11,"row":2})");
    handleAddBlock(inputJson);
    handleAddBlock(outputJson);

    lastPresetFile = juce::File{};
    currentPresetIndex = -1;
    scenes.clear();
    Scene defaultScene;
    defaultScene.name = "Scene 1";
    captureIntoScene(defaultScene, blockNodeMap, processor->getGraph());
    scenes.push_back(defaultScene);
    activeSceneIndex = 0;

    // Clear preset-level MIDI mappings
    if (processor != nullptr)
    {
        processor->getMidiMapper().loadPresetMappings(juce::var());
        emitMidiMappings();
    }

    persistPresetInfo();

    // Reset grid size to the UI default when starting a new session so the
    // new preset doesn't inherit the previous session's custom dimensions.
    gridCols = 12;
    gridRows = 6;

    sendGraphState();
    emitGridState();
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

void StellarrBridge::handleRenamePreset(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    auto newName = obj->getProperty("name").toString().trim();
    if (index < 0 || index >= presetFiles.size() || newName.isEmpty()) return;

    auto oldFile = presetDirectory.getChildFile(presetFiles[index]);
    auto newFile = presetDirectory.getChildFile(newName + ".stellarr");

    if (newFile.existsAsFile() || !oldFile.existsAsFile()) return;

    if (oldFile.moveFileTo(newFile))
    {
        // Update tracking if this was the active preset
        if (index == currentPresetIndex)
            lastPresetFile = newFile;

        handleGetPresetList();

        // Find new index after re-sorting
        currentPresetIndex = -1;
        for (int i = 0; i < presetFiles.size(); ++i)
        {
            if (presetFiles[i] == newFile.getFileName())
            {
                currentPresetIndex = i;
                break;
            }
        }

        sendPresetList();
        persistPresetInfo();
    }
}

void StellarrBridge::handleDeletePreset(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= presetFiles.size()) return;

    auto file = presetDirectory.getChildFile(presetFiles[index]);
    if (!file.existsAsFile()) return;

    if (file.deleteFile())
    {
        bool wasActive = (index == currentPresetIndex);

        handleGetPresetList();

        if (wasActive)
        {
            // Deleted the active preset — clear active state
            currentPresetIndex = -1;
            lastPresetFile = juce::File{};
        }
        else if (currentPresetIndex > index)
        {
            // Active preset shifted down
            currentPresetIndex--;
        }
        // else: active preset was before deleted one, index unchanged

        sendPresetList();
        persistPresetInfo();
    }
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

// -- Grid dimensions ----------------------------------------------------------

void StellarrBridge::emitGridState()
{
    auto* detail = new juce::DynamicObject();
    detail->setProperty("columns", gridCols);
    detail->setProperty("rows", gridRows);
    emitToJs("gridState", detail);
}

void StellarrBridge::handleSetGridSize(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto cols = obj->getProperty("columns");
    auto rows = obj->getProperty("rows");
    if (!(cols.isInt() || cols.isInt64() || cols.isDouble())) return;
    if (!(rows.isInt() || rows.isInt64() || rows.isDouble())) return;

    gridCols = juce::jmax(1, static_cast<int>(cols));
    gridRows = juce::jmax(1, static_cast<int>(rows));

    // Autosave so the new dimensions survive a restart, matching how block
    // edits persist.
    handleSaveSessionQuiet();
}
