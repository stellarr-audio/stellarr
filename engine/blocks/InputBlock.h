#pragma once
#include "Block.h"

namespace stellarr
{

class InputBlock final : public Block
{
public:
    InputBlock() : Block("Input", 0, 2) {}

    BlockType getBlockType() const override { return BlockType::input; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override
    {
        // Audio is injected by the graph's audio input node.
        // This block acts as a pass-through marker.
    }
};

} // namespace stellarr
