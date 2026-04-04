#pragma once
#include "../StellarrBridge.h"
#include "../blocks/PluginBlock.h"

inline void captureIntoScene(StellarrBridge::Scene& scene,
                             const std::map<juce::String, juce::AudioProcessorGraph::NodeID>& blockNodeMap,
                             juce::AudioProcessorGraph& graph)
{
    scene.blockStateMap.clear();
    scene.blockBypassMap.clear();
    for (auto& [blockId, nodeId] : blockNodeMap)
    {
        if (auto* node = graph.getNodeForId(nodeId))
        {
            if (auto* pb = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
            {
                scene.blockStateMap[blockId] = pb->getActiveStateIndex();
                scene.blockBypassMap[blockId] = pb->isBypassed();
            }
        }
    }
}
