#pragma once
#include "Block.h"

namespace stellarr
{

// Pass-through placeholder. Effect processing logic is not yet implemented.
class FxBlock final : public Block
{
public:
    FxBlock() : Block("FX", 2, 2) {}

    BlockType getBlockType() const override { return BlockType::fx; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

} // namespace stellarr
