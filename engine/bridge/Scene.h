#pragma once
#include <juce_core/juce_core.h>
#include <map>

namespace stellarr::bridge
{
    // A snapshot of per-block state index + bypass status, used to capture and
    // restore a named "scene" within a session. Owned by SceneHandler; copied
    // into / out of the session JSON by SessionSerializer.
    struct Scene
    {
        juce::String name;
        std::map<juce::String, int> blockStateMap;
        std::map<juce::String, bool> blockBypassMap;
    };
}
