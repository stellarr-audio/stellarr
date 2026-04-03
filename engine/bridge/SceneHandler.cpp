#include "../StellarrBridge.h"
#include "../StellarrProcessor.h"
#include "../blocks/PluginBlock.h"

// -- Scene capture helpers ----------------------------------------------------

struct SceneCapture {
    std::map<juce::String, int> stateMap;
    std::map<juce::String, bool> bypassMap;
};

static SceneCapture captureScene(
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
    auto cap = captureScene(blockNodeMap, graph);
    scene.blockStateMap = cap.stateMap;
    scene.blockBypassMap = cap.bypassMap;
}

// -- Scene event handlers -----------------------------------------------------

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

void StellarrBridge::handleAddScene()
{
    if (processor == nullptr || static_cast<int>(scenes.size()) >= maxScenes) return;

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

    // Save current live states and update outgoing scene
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
        auto* pb = findPluginBlock(blockId);
        if (pb == nullptr) continue;

        int clampedIdx = std::min(stateIdx, pb->getNumStates() - 1);
        if (clampedIdx >= 0)
        {
            pb->recallState(clampedIdx);

            auto bypassIt = scene.blockBypassMap.find(blockId);
            if (bypassIt != scene.blockBypassMap.end())
                pb->setBypassed(bypassIt->second);

            emitBlockStates(blockId, pb);
            emitBlockParams(blockId, pb);
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
