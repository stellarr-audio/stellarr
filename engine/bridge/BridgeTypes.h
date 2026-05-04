#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <map>

namespace stellarr::bridge
{
    // Bridge-level type aliases shared across the bridge layer. Lives at the
    // top-level bridge namespace (not under internal/) so per-handler context
    // structs can reference these types without pulling in helper-only headers.
    //
    // Convention for free functions in stellarr::bridge::internal::
    // data-source args precede query/operation args, e.g.
    //   findBlock(const BlockNodeMap&, StellarrProcessor&, const juce::String& blockId)

    using BlockNodeMap = std::map<juce::String, juce::AudioProcessorGraph::NodeID>;
}
