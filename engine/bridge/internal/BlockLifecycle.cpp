#include "BlockLifecycle.h"
#include "../../StellarrProcessor.h"
#include "../../blocks/PluginBlock.h"

namespace stellarr::bridge::internal
{
    void connectIOBlock(
        StellarrProcessor& processor,
        const juce::String& type,
        juce::AudioProcessorGraph::NodeID nodeId,
        juce::AudioProcessorGraph::UpdateKind update)
    {
        // Non-IO types (plugin/vst) share this entry point via paste / restore
        // paths but don't wire to the IO graph nodes — bail before touching the
        // default bypass so a plugin-only restore or paste doesn't silently
        // sever audio on a graph that still relies on the ctor's
        // audioInput -> audioOutput passthrough.
        if (type != "input" && type != "output") return;

        // Tear down the default audioInput -> audioOutput bypass before wiring an
        // IO block. StellarrProcessor's ctor adds that direct connection so a fresh
        // app boot still passes audio; once a real input/output block is added,
        // it must go — otherwise the dry signal leaks past the block chain (and
        // outside any Block::level / Block::bypass logic). disconnectBlocks is a
        // no-op if the connection is already gone, so it's safe to call from
        // every IO-wiring path.
        processor.disconnectBlocks(processor.getAudioInputNodeId(),
                                   processor.getAudioOutputNodeId(), update);

        if (type == "input")
        {
            processor.connectBlocks(processor.getAudioInputNodeId(), nodeId, 2, update);
            processor.getGraph().addConnection({
                {processor.getMidiInputNodeId(), juce::AudioProcessorGraph::midiChannelIndex},
                {nodeId, juce::AudioProcessorGraph::midiChannelIndex}
            }, update);
        }
        else if (type == "output")
        {
            processor.connectBlocks(nodeId, processor.getAudioOutputNodeId(), 2, update);
            processor.getGraph().addConnection({
                {nodeId, juce::AudioProcessorGraph::midiChannelIndex},
                {processor.getMidiOutputNodeId(), juce::AudioProcessorGraph::midiChannelIndex}
            }, update);
        }
    }

    void restoreBlockPlugin(
        StellarrProcessor& processor,
        juce::AudioProcessorGraph::NodeID nodeId,
        const juce::String& pluginId,
        const juce::String& savedPluginName)
    {
        if (pluginId.isEmpty()) return;

        auto* node = processor.getGraph().getNodeForId(nodeId);
        if (node == nullptr) return;

        if (auto* pluginBlock = dynamic_cast<stellarr::PluginBlock*>(node->getProcessor()))
        {
            juce::String errorMessage;
            auto instance = processor.getPluginManager().createPluginInstance(
                pluginId, processor.getSampleRate(),
                processor.getBlockSize(), errorMessage);

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
}
