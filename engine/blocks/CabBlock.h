#pragma once
#include "Block.h"

namespace stellarr
{

// Pass-through placeholder. Cabinet simulation logic is not yet implemented.
class CabBlock final : public Block
{
public:
    CabBlock() : Block("Cab", 2, 2) {}

    BlockType getBlockType() const override { return BlockType::cab; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

} // namespace stellarr
