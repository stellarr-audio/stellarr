#pragma once
#include <juce_core/juce_core.h>
#include "BridgeTypes.h"
#include "IBridgeEmitter.h"

class StellarrProcessor;

namespace stellarr::bridge
{
    // Per-handler context wired up by StellarrBridge after the processor is
    // attached (see StellarrBridge::setProcessor). Mirrors the pattern
    // established by UpdateHandler / ParamHandler / GraphHandler / MidiHandler /
    // SceneHandler.
    struct InputBlockHandlerContext
    {
        StellarrProcessor& processor;
        const BlockNodeMap& blockNodeMap;
        IBridgeEmitter& emit;
    };

    // Bridge handlers for test-tone toggle/sample-pick and tuner enable/disable.
    // Owns the tunerActive flag — cross-handler reads go via isTunerActive().
    class InputBlockHandler
    {
    public:
        explicit InputBlockHandler(InputBlockHandlerContext ctx);

        // Dispatch handlers — called from the StellarrBridge dispatch table.
        void handleToggleTestTone(const juce::var& json);
        void handleGetTestToneSamples();
        void handleSetTestToneSample(const juce::var& json);
        void handleSetTunerEnabled(const juce::var& json);

        // Apply the tuner enabled state to every InputBlock (enable pitch
        // analysis) and OutputBlock (mute output) in the graph. Used by
        // handleSetTunerEnabled and by MidiHandler when a tuner-toggle CC
        // arrives.
        void setTunerEnabledOnAllBlocks(bool enabled);

        bool isTunerActive() const { return tunerActive; }

    private:
        InputBlockHandlerContext ctx;
        bool tunerActive = false;
    };
}
