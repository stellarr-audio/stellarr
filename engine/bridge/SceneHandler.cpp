#include "../StellarrProcessor.h"
#include "SceneCapture.h"

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

// Returns true when switching from `outgoing` to `incoming` requires the slow
// "rewire" path (capture + setStateInformation), false when the swap is
// purely a Stellarr-level change (mix/balance/level/bypass) that can be
// applied without touching plugin binary state.
//
// The check is structural: any per-block effective State index difference
// implies the plugin needs different binary parameters loaded; if every
// block resolves to the same effective State index, the plugins already
// hold the right binaries and only Stellarr-level fields need updating.
//
// Effective index = the raw stateMap value clamped to the block's current
// state count, matching the clamp `handleRecallScene` applies before recall.
// Without clamping here, scenes with stale out-of-range indices left over
// from a previous deletion would incorrectly trip the rewire path even
// though they recall to the same State as their sibling scenes.
static bool sceneRewireRequired(const StellarrBridge::Scene& outgoing,
                                const StellarrBridge::Scene& incoming,
                                const std::map<juce::String, juce::AudioProcessorGraph::NodeID>& blockNodeMap,
                                juce::AudioProcessorGraph& graph)
{
    auto effective = [&blockNodeMap, &graph](const juce::String& blockId, int rawIdx) -> int
    {
        auto it = blockNodeMap.find(blockId);
        if (it == blockNodeMap.end()) return -1;
        auto* node = graph.getNodeForId(it->second);
        if (node == nullptr) return -1;
        auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor());
        if (pb == nullptr) return -1;
        const int n = pb->getNumStates();
        if (n <= 0) return -1;
        return std::min(rawIdx, n - 1);
    };

    for (const auto& [blockId, idx] : incoming.blockStateMap)
    {
        auto it = outgoing.blockStateMap.find(blockId);
        const int outRaw = (it != outgoing.blockStateMap.end()) ? it->second : -1;
        if (effective(blockId, outRaw) != effective(blockId, idx)) return true;
    }
    for (const auto& [blockId, idx] : outgoing.blockStateMap)
    {
        if (incoming.blockStateMap.find(blockId) == incoming.blockStateMap.end())
            return true;
    }
    return false;
}

void StellarrBridge::handleRecallScene(const juce::var& json)
{
    if (processor == nullptr) return;
    auto* obj = json.getDynamicObject();
    if (obj == nullptr) return;

    auto index = static_cast<int>(obj->getProperty("index"));
    if (index < 0 || index >= static_cast<int>(scenes.size())) return;

    // Decide whether this swap requires plugin binary rewiring or only
    // Stellarr-level field updates. With no active scene, treat as rewire so
    // the destination scene's binaries get applied at least once.
    const bool willRewire = (activeSceneIndex < 0
                          || activeSceneIndex >= static_cast<int>(scenes.size()))
        ? true
        : sceneRewireRequired(scenes[static_cast<size_t>(activeSceneIndex)],
                              scenes[static_cast<size_t>(index)],
                              blockNodeMap,
                              processor->getGraph());

    if (willRewire)
    {
        // Slow capture: serialise plugin binary state into each block's active
        // State slot so unsaved tweaks survive the upcoming setStateInformation
        // pushes. Only worth doing on the rewire path — captureCurrentState()
        // calls plugin->getStateInformation() per block, which is exactly what
        // the fast path is built to avoid.
        for (auto& [blockId, nodeId] : blockNodeMap)
        {
            if (auto* node = processor->getGraph().getNodeForId(nodeId))
                if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
                    pb->saveCurrentState();
        }
    }

    // Cheap capture: refresh the outgoing scene's blockStateMap and
    // blockBypassMap to reflect any tweaks made while it was active. Runs on
    // both paths — only the slow plugin binary capture above is gated on
    // willRewire. Without this, fast-path A → B → A loses bypass tweaks made
    // while A was active because A's stored blockBypassMap is reapplied
    // verbatim on return.
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
            if (willRewire)
                pb->recallState(clampedIdx);          // capture + setStateInformation
            else
                pb->recallStateStellarrOnly(clampedIdx); // index + Stellarr-level only

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
