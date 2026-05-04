#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

class StellarrProcessor;

namespace stellarr::bridge::internal
{
    // Wire an input/output block's audio + MIDI to the processor's IO nodes.
    // Tears down the default audioInput->audioOutput passthrough so the dry
    // signal doesn't leak past the block chain. Safe to call from add /
    // paste / restore paths.
    void connectIOBlock(
        StellarrProcessor& processor,
        const juce::String& type,
        juce::AudioProcessorGraph::NodeID nodeId,
        juce::AudioProcessorGraph::UpdateKind update = juce::AudioProcessorGraph::UpdateKind::sync);

    // Restore a saved plugin into a freshly-created PluginBlock, marking
    // the block as missing if the plugin can't be instantiated.
    void restoreBlockPlugin(
        StellarrProcessor& processor,
        juce::AudioProcessorGraph::NodeID nodeId,
        const juce::String& pluginId,
        const juce::String& savedPluginName);
}
