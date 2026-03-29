#pragma once
#include "Block.h"

namespace stellarr
{

class OutputBlock final : public Block
{
public:
    OutputBlock() : Block("Output", 2, 0) {}

    BlockType getBlockType() const override { return BlockType::output; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        // Audio is forwarded to the graph's audio output node.
        // This block acts as a pass-through marker.
    }
};

} // namespace stellarr
