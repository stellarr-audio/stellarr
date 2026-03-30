#pragma once
#include "Block.h"

namespace stellarr
{

// Pass-through placeholder. Amp modelling logic is not yet implemented.
class AmpBlock final : public Block
{
public:
    AmpBlock() : Block("Amp", 2, 2) {}

    BlockType getBlockType() const override { return BlockType::amp; }

    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
};

} // namespace stellarr
