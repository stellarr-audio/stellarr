#pragma once
#include "../blocks/PluginBlock.h"
#include "Scene.h"

inline void captureIntoScene(stellarr::bridge::Scene& scene,
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
