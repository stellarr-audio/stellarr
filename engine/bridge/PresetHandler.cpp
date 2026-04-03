#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../blocks/InputBlock.h"
#include "../blocks/OutputBlock.h"
#include "../blocks/PluginBlock.h"

// -- Scene capture (needed for serialization) ---------------------------------

struct PresetSceneCapture {
    std::map<juce::String, int> stateMap;
    std::map<juce::String, bool> bypassMap;
};

static PresetSceneCapture captureSceneForPreset(
    const std::map<juce::String, juce::AudioProcessorGraph::NodeID>& blockNodeMap,
    juce::AudioProcessorGraph& graph)
{
    PresetSceneCapture cap;
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

static void captureIntoSceneForPreset(StellarrBridge::Scene& scene,
                                       const std::map<juce::String, juce::AudioProcessorGraph::NodeID>& blockNodeMap,
                                       juce::AudioProcessorGraph& graph)
{
    auto cap = captureSceneForPreset(blockNodeMap, graph);
    scene.blockStateMap = cap.stateMap;
    scene.blockBypassMap = cap.bypassMap;
}

// -- Session serialization ----------------------------------------------------

juce::var StellarrBridge::serialiseSession() const
{
    if (processor == nullptr) return {};

    auto* session = new juce::DynamicObject();
    session->setProperty("version", 1);

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
        captureIntoSceneForPreset(const_cast<StellarrBridge*>(this)->scenes[static_cast<size_t>(activeSceneIndex)],
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
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = processor->getGraph().getNodeForId(nodeId))
            if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                pluginBlock->closePluginEditor();
    }

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
        captureIntoSceneForPreset(defaultScene, blockNodeMap, processor->getGraph());
        scenes.push_back(defaultScene);
        activeSceneIndex = 0;
    }

    sendGraphState();
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
    captureIntoSceneForPreset(defaultScene, blockNodeMap, processor->getGraph());
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
